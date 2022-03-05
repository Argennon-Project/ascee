// Copyright (c) 2021-2022 aybehrouz <behrouz_ayati@yahoo.com>. All rights
// reserved. This file is part of the C++ implementation of the Argennon smart
// contract Execution Environment (AscEE).
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option) any
// later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License
// for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

#ifndef ARGENNON_EXEC_SCHEDULER_H
#define ARGENNON_EXEC_SCHEDULER_H

#include <unordered_set>
#include <atomic>
#include <cassert>

#include "core/primitives.h"
#include "core/info.h"
#include "util/BlockingQueue.hpp"
#include "storage/ChunkIndex.h"
#include "ascee/executor/Executor.h"

namespace argennon::ave {


class RequestScheduler;

class DagNode {
public:
    int_fast32_t decrementInDegree() {
        assert(inDegree > 0);
        return --inDegree;
    }

    void incrementInDegree() { ++inDegree; }

    ascee::runtime::AppRequest& getAppRequest() {
        return request;
    }

    int_fast32_t getInDegree() const {
        return inDegree;
    }

    auto& adjacentNodes() {
        return adjList;
    }

    bool isAdjacent(AppRequestIdType other) const {
        return adjList.contains(other);
    }

    explicit DagNode(AppRequestInfo&& data, const RequestScheduler* scheduler);

private:
    ascee::runtime::AppRequest request;
    const std::unordered_set<AppRequestIdType> adjList;
    std::atomic<int_fast32_t> inDegree = 0;
};

template<class Dag>
class VerifierCluster {
    using AccessType = AccessBlockInfo::Access::Type;
public:
    explicit VerifierCluster(Dag* dag) : dag(dag) { members.reserve(16); }

    void merge(const VerifierCluster& c) {
        if (c.type == AccessType::writable) {
            util::mergeInsert(members, c.members);
        } else {
            members.insert(members.end(), c.members.begin(), c.members.end());
        }
    }

    void insert(AppRequestIdType requestID, AccessBlockInfo::Access accessType) {
        type = accessType;
        if (type == AccessType::writable) util::mergeInsert(members, {requestID});
        else members.emplace_back(requestID);
    }

    void finalize() {
        if (type == AccessType::writable) {
            // for verifying a clique we have to verify that a path exist in the dag that passes through all clique
            // vertices.

            // we keep vertices of cliques sorted. Without sorting, this function can not guarantee that a path
            // exists through all vertices.
            for (int32_fast i = 0; i + 1 < members.size(); ++i) {
                registerAdjacency(dag, members[i], members[i + 1]);
            }

            printf("( ");
            for (const auto& member: members) printf("%ld ", member);
            printf(")\n");
        }
    }

    void addDependency(AppRequestIdType u) {
        // when the cluster represents a clique (i.e. it's writable) we just make sure that there is a path between
        // every member of the clique to u.
        if (type == AccessType::writable) {
            auto next = std::upper_bound(members.begin(), members.end(), u);
            auto previous = next - 1;
            if (next == members.end()) {
                registerAdjacency(dag, *previous, u);
            } else if (next == members.begin()) {
                registerAdjacency(dag, u, *next);
            } else {
                registerAdjacency(dag, *previous, u);
                registerAdjacency(dag, u, *next);
            }
        } else {
            for (const auto& member: members) {
                addDependency(dag, member, u);
            }
        }

        printf("[ ");
        for (const auto& member: members) printf("%ld ", member);
        printf("] * %ld\n", u);
    }

    static
    void addDependency(Dag* dag, AppRequestIdType u, AppRequestIdType v) {
        if (u < v) registerAdjacency(dag, u, v);
        else registerAdjacency(dag, v, u);
    }

private:
    Dag* dag;
    AccessBlockInfo::Access type = AccessType::check_only;
    std::vector<AppRequestIdType> members;

    static
    void registerAdjacency(Dag* dag, AppRequestIdType u, AppRequestIdType v) {
        if (!dag->isAdjacent(u, v)) {
            throw std::invalid_argument("missing edge:{" + std::to_string(u) + "," + std::to_string(v) +
                                        "} in the dependency graph");
        }
    }
};

/// works fine even if the graph is not a dag and contains loops
/// RequestSchedulers are created per block
class RequestScheduler {
public:
    ascee::runtime::AppRequest* nextRequest();

    void submitResult(AppRequestIdType reqID, int statusCode);

    void findCollisions(full_id chunkID,
                        const std::vector<int32>& sortedOffsets,
                        const std::vector<AccessBlockInfo>& accessBlocks);

    /// this function is thread-safe as long as all used `id`s are distinct
    auto& requestAt(AppRequestIdType id);

    /// this function is thread-safe as long as all used `id`s are distinct
    void addRequest(AppRequestInfo&& data);

    /// this function should be called after all requests are added. (using addRequest() or requestAt())
    void finalizeRequest(AppRequestIdType id);

    void buildExecDag();

    [[nodiscard]]
    AppRequestInfo::AccessMapType sortAccessBlocks(int workersCount);

    explicit RequestScheduler(int32_fast totalRequestCount, asa::ChunkIndex& heapIndex);

    [[nodiscard]]
    ascee::runtime::HeapModifier getModifierFor(AppRequestIdType requestID) const;

    explicit operator std::string() const;

    template<class Cluster, class Dag>
    static void findCollisionCliques(std::vector<int32>&& sortedOffsets, std::vector<AccessBlockInfo>&& accessBlocks,
                                     Dag* dag) {
        std::vector<Cluster> clusters(sortedOffsets.size(), Cluster(dag));
        for (int32_fast i = 0; i < sortedOffsets.size(); ++i) {
            auto accessType = accessBlocks[i].accessType;
            auto offset = sortedOffsets[i];
            // with this simple if we can skip non-accessible size blocks because based on their offset they will always
            // be at the start of the list.
            if (offset == -3 || accessType == AccessBlockInfo::Access::Type::check_only) continue;

            clusters[i].insert(accessBlocks[i].requestID, accessType);
            auto end = (offset == -1 || offset == -2) ? 0 : offset + accessBlocks[i].size;

            bool skipped = false;
            for (int32_fast j = i + 1; j < accessBlocks.size() && sortedOffsets[j] < end; ++j) {
                auto collidingEnd = sortedOffsets[j] + accessBlocks[j].size;
                if (!canMerge(accessBlocks[i], offset, accessBlocks[j], sortedOffsets[j]) || end > collidingEnd) {
                    // canMerge returns true for same additive blocks, so we don't need to check that here. In other
                    // word we merge additive blocks that do not collide.
                    if (accessType.collides(accessBlocks[j].accessType)) {
                        clusters[i].addDependency(accessBlocks[j].requestID);
                    }
                } else {
                    clusters[j].merge(clusters[i]);
                    skipped = true;
                    if (end < collidingEnd) {
                        auto lowerBound = std::lower_bound(sortedOffsets.begin() + i + 1, sortedOffsets.end(), end);
                        if (lowerBound != sortedOffsets.end() && *lowerBound < collidingEnd) {
                            // split colliding access block
                            int32_fast k;
                            for (k = i + 1; sortedOffsets[k] < end && k < sortedOffsets.size(); ++k) {
                                sortedOffsets[k - 1] = sortedOffsets[k];
                                accessBlocks[k - 1] = accessBlocks[k];
                                clusters[k - 1] = std::move(clusters[k]);
                            }
                            sortedOffsets[k - 1] = end;
                            accessBlocks[k - 1] = accessBlocks[j - 1];
                            accessBlocks[j - 1].size = end - sortedOffsets[j - 1];
                            accessBlocks[k - 1].size -= accessBlocks[j - 1].size;
                            clusters[k - 1] = Cluster(dag);
                            --i;
                        }
                    }
                    break;
                }
            }
            if (!skipped) clusters[i].finalize();
        }
    }

    template<class Cluster, class Dag>
    static void findResizingCollisions(const std::vector<int32>& sortedOffsets,
                                       const std::vector<AccessBlockInfo>& accessBlocks,
                                       Dag* dag,
                                       const std::function<int32_fast()>& getSizeLowerBound) {
        auto writersEndIt = std::upper_bound(sortedOffsets.begin(), sortedOffsets.end(), -1);
        if (writersEndIt == sortedOffsets.end()) return;

        auto writersBeginIt = std::lower_bound(sortedOffsets.begin(), writersEndIt, -1);
        if (writersBeginIt == writersEndIt) return;

        int32_fast sizeWritersEnd = writersEndIt - sortedOffsets.begin();
        int32_fast sizeWritersBegin = writersBeginIt - sortedOffsets.begin();
        int32_fast lowerBound = getSizeLowerBound();

        for (int32_fast i = sizeWritersEnd; i < accessBlocks.size(); ++i) {
            auto offset = sortedOffsets[i];
            assert(offset >= 0);
            auto end = offset + accessBlocks[i].size;
            auto reqID = accessBlocks[i].requestID;

            // end > upperBound will never occur, since those transactions must not be included in a block.
            if (end > lowerBound) {
                for (int32_fast j = sizeWritersBegin; j < sizeWritersEnd; ++j) {
                    if (reqID != accessBlocks[j].requestID) {
                        auto newSize = accessBlocks[j].size;
                        bool collision = newSize > 0 ? offset < newSize : end > -newSize;
                        if (collision) Cluster::addDependency(dag, reqID, accessBlocks[j].requestID);
                    }
                }
            }
        }
    }

    /**
     * verifies that the execution dag of requests properly considers all collisions between requests. Collisions
     * are computed using cluster-product algorithm.
     * @param chunkID
     * @param sortedOffsets when possible, use std::move for this parameter
     * @param accessBlocks when possible, use std::move for this parameter
     */
    void checkCollisions(full_id chunkID,
                         std::vector<int32> sortedOffsets, std::vector<AccessBlockInfo> accessBlocks) {
        findResizingCollisions<VerifierCluster<RequestScheduler>>(sortedOffsets, accessBlocks, this,
                                                                  [this, chunkID] {
                                                                      return heapIndex.getSizeLowerBound(chunkID);
                                                                  });
        findCollisionCliques<VerifierCluster<RequestScheduler>>(std::move(sortedOffsets), std::move(accessBlocks),
                                                                this);
    }

    [[nodiscard]]
    bool isAdjacent(AppRequestIdType u, AppRequestIdType v) const {
        printf("e:(%ld,%ld)\n", u, v);
        return nodeIndex[u]->isAdjacent(v);
    }

private:
    asa::ChunkIndex& heapIndex;
    std::atomic<int_fast32_t> remaining;
    util::BlockingQueue<DagNode*> zeroQueue;
    std::unique_ptr<std::unique_ptr<DagNode>[]> nodeIndex;
    std::vector<AppRequestInfo::AccessMapType> memoryAccessMaps;

    void registerDependency(AppRequestIdType u, AppRequestIdType v);

    void injectDigest(Digest digest, std::string& httpRequest) {}

    static bool
    canMerge(const AccessBlockInfo& left, int32 leftOffset, const AccessBlockInfo& right, int32 rightOffset);
};

} // namespace argennon::ave
#endif // ARGENNON_EXEC_SCHEDULER_H
