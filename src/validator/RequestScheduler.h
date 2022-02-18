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

#include "arg/primitives.h"
#include "arg/info.h"
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
class DagVerifier {
public:
    explicit DagVerifier(const Dag& dag) : dag(dag) {}

    void registerDependency(const std::vector<AppRequestIdType>& cluster, AppRequestIdType v) {
        printf("[ ");
        for (const auto& u: cluster) {
            verifyIsAdjacent(u, v);
            printf("%ld ", u);
        }
        printf("]->(%ld)\n", v);
    }

    void registerDependency(AppRequestIdType u, AppRequestIdType v) {
        verifyIsAdjacent(u, v);

        printf("%ld->%ld\n", u, v);
    }

    void registerClique(const std::vector<AppRequestIdType>& clique) {
        printf("(");
        for (int i = 0; i < clique.size() - 1; ++i) {
            verifyIsAdjacent(clique[i], clique[i + 1]);
            printf("%ld ", clique[i]);
        }
        printf("%ld)\n", clique.back());
    }

private:
    const Dag& dag;

    void verifyIsAdjacent(AppRequestIdType u, AppRequestIdType v) {
        bool adjacent = u < v ? dag.isAdjacent(u, v) : dag.isAdjacent(v, u);
        if (!adjacent) {
            throw std::invalid_argument("missing {" + std::to_string(u) + "," + std::to_string(v) +
                                        "} edge in the dependency graph");
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

    explicit RequestScheduler(int_fast32_t totalRequestCount, asa::ChunkIndex& heapIndex);

    [[nodiscard]]
    ascee::runtime::HeapModifier getModifierFor(AppRequestIdType requestID) const;

    explicit operator std::string() const {
        std::string result;
        for (int i = 0; i < remaining; ++i) {
            result += std::to_string(nodeIndex[i]->adjacentNodes().size()) + "=";
        }
        return result;
    }

    template<class DependencyGraph>
    static void findCollisionCliques(std::vector<int32>&& sortedOffsets, std::vector<AccessBlockInfo>&& accessBlocks,
                                     DependencyGraph& graph) {
        std::vector<std::vector<AppRequestIdType>> clusters(sortedOffsets.size());
        for (int32_fast i = 0; i < sortedOffsets.size(); ++i) {
            auto accessType = accessBlocks[i].accessType;
            auto offset = sortedOffsets[i];

            if (offset == -3) continue;

            clusters[i].emplace_back(accessBlocks[i].requestID);
            auto end = (offset == -1 || offset == -2) ? 0 : offset + accessBlocks[i].size;
            bool skipped = false;
            for (int32_fast j = i + 1; sortedOffsets[j] < end && j < accessBlocks.size(); ++j) {
                auto collidingEnd = sortedOffsets[j] + accessBlocks[j].size;
                if (end > collidingEnd || !accessBlocks[j].accessType.merges(accessType)) {
                    if (hasCollision(accessBlocks[i], offset, accessBlocks[j], sortedOffsets[j])) {
                        graph.registerDependency(clusters[i], accessBlocks[j].requestID);
                    }
                } else {
                    clusters[j].insert(clusters[j].end(), clusters[i].begin(), clusters[i].end());
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
                            clusters[k - 1].clear();
                            --i;
                        }
                    }
                    break;
                }
            }
            if (!skipped && clusters[i].size() > 1 && accessType.collides(accessType)) {
                graph.registerClique(clusters[i]);
            }
        }
    }

    template<class DependencyGraph>
    void findResizingCollisions(full_id chunkID, const std::vector<int32>& sortedOffsets,
                                const std::vector<AccessBlockInfo>& accessBlocks, DependencyGraph& graph) {
        int32_fast sizeWritersBegin = 0, sizeWritersEnd = 0;
        bool inSizeWriterList = false;
        int32_fast lowerBound = 0;
        for (int32_fast i = 0; i < accessBlocks.size(); ++i) {
            auto offset = sortedOffsets[i];
            auto reqID = accessBlocks[i].requestID;

            // with this simple if we can skip non-accessible size blocks because based on their offset they will always
            // be at the start of the list.
            if (offset == -3) continue;

            auto end = (offset == -1 || offset == -2) ? 0 : offset + accessBlocks[i].size;

            // finding the index interval of chunk resizing info blocks in the input vector: for all of them we
            // have offset == -1
            if (!inSizeWriterList && offset == -1) {
                inSizeWriterList = true;
                sizeWritersBegin = i;
            } else if (inSizeWriterList && offset != -1) {
                inSizeWriterList = false;
                sizeWritersEnd = i;
                lowerBound = heapIndex.getSizeLowerBound(chunkID);
            }

            // end > upperBound will never occur, since those transactions must not be included in a block.
            if (end > lowerBound) {
                for (int32_fast j = sizeWritersBegin; j < sizeWritersEnd; ++j) {
                    if (reqID != accessBlocks[j].requestID) {
                        auto newSize = accessBlocks[j].size;
                        bool collision = newSize > 0 ? offset < newSize : end > -newSize;
                        if (collision) graph.registerDependency(reqID, accessBlocks[j].requestID);
                    }
                }
            }
        }
    }

    void checkCollisions(full_id chunkID, std::vector<int32> sortedOffsets,
                         std::vector<AccessBlockInfo> accessBlocks) {
        DagVerifier verifier(*this);
        findResizingCollisions(chunkID, sortedOffsets, accessBlocks, verifier);
        findCollisionCliques(std::move(sortedOffsets), std::move(accessBlocks), verifier);
    }

    [[nodiscard]]
    bool isAdjacent(AppRequestIdType u, AppRequestIdType v) const {
        return nodeIndex[u]->isAdjacent(v);
    }

private:
    asa::ChunkIndex& heapIndex;
    std::atomic<int_fast32_t> remaining;
    util::BlockingQueue<DagNode*> zeroQueue;
    std::unique_ptr<std::unique_ptr<DagNode>[]> nodeIndex;
    std::vector<AppRequestInfo::AccessMapType> memoryAccessMaps;

    void registerDependency(AppRequestIdType u, AppRequestIdType v);

    void injectDigest(util::Digest digest, std::string& httpRequest) {}

    static bool
    hasCollision(const AccessBlockInfo& left, int32 leftOffset, const AccessBlockInfo& right, int32 rightOffset);
};

} // namespace argennon::ave
#endif // ARGENNON_EXEC_SCHEDULER_H
