// Copyright (c) 2021 aybehrouz <behrouz_ayati@yahoo.com>. All rights reserved.
// This file is part of the C++ implementation of the Argennon smart contract
// Execution Environment (AscEE).
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

#ifndef ASCEE_EXEC_SCHEDULER_H
#define ASCEE_EXEC_SCHEDULER_H

#include <argc/types.h>
#include <util/BlockingQueue.h>
#include <unordered_set>
#include <cassert>
#include "heap/PageCache.h"
#include "heap/Modifier.h"
#include "FailureManager.h"
#include "loader/BlockLoader.h"
#include "loader/AppLoader.h"
#include <atomic>
#include <utility>

namespace ascee::runtime {

using ReqIdType = AppRequestRawData::IdType;

struct AppRequest {
    ReqIdType id;
    long_id calledAppID;
    std::string httpRequest;
    int_fast32_t gas;
    heap::Modifier modifier;
    std::unordered_map<long_id, dispatcher_ptr> appTable;
    FailureManager failureManager;
    Digest headerDigest;
    Digest fullDigest;
};

struct AppResponse {
    ReqIdType txID;
    int statusCode;
    std::string response;
};

class DagNode {
public:
    int_fast32_t decrementInDegree() {
        assert(inDegree > 0);
        return --inDegree;
    }

    void incrementInDegree() { ++inDegree; }

    AppRequest& getAppRequest() {
        return request;
    }

    int_fast32_t getInDegree() const {
        return inDegree;
    }

    auto& adjacentNodes() {
        return adjList;
    }

    bool isAdjacent(ReqIdType other) const {
        return adjList.contains(other);
    }

    // Members are initialized in left-to-right order as they appear in this class's base-specifier list.
    explicit DagNode(AppRequestRawData&& data) :
            adjList(std::move(data.adjList)),
            request{
                    data.id,
                    data.calledAppID,
                    std::move(data.httpRequest),
                    data.gas,
                    {},
                    AppLoader::global->createAppTable(data.appAccessList),
                    FailureManager(std::move(data.stackSizeFailures), std::move(data.cpuTimeFailures)),
                    std::move(data.headerDigest)
            } {}

private:
    AppRequest request;
    const std::unordered_set<ReqIdType> adjList;
    std::atomic<int_fast32_t> inDegree = 0;
};

/// works fine even if the graph is not a dag and contains loops
class RequestScheduler {
public:
    AppRequest* nextRequest();

    void submitResult(const AppResponse& result);

    void addMemoryAccessList(long_id appID, long_id chunkID, const std::vector<AccessBlock>& sortedAccessBlocks);

    /// this function is thread-safe as long as all used `id`s are distinct
    auto& requestAt(long id) {
        return nodeIndex.at(id);
    }

    /// this function is thread-safe as long as all used `id`s are distinct
    void addRequest(long id, AppRequestRawData&& data) {
        nodeIndex.at(id) = std::make_unique<DagNode>(std::move(data));
    }

    void buildDag() {
        for (const auto& node: nodeIndex) {
            if (node->getInDegree() == 0) zeroQueue.enqueue(node.get());

            for (const auto adjID: node->adjacentNodes()) {
                nodeIndex[adjID]->incrementInDegree();
            }
        }
    }

    explicit RequestScheduler(int_fast32_t totalRequestCount, heap::PageCache::ChunkIndex& heapIndex) :
            heapIndex(heapIndex),
            count(totalRequestCount),
            nodeIndex(totalRequestCount) {}

private:
    heap::PageCache::ChunkIndex& heapIndex;
    std::atomic<int_fast32_t> count;
    BlockingQueue<DagNode*> zeroQueue;
    std::vector<std::unique_ptr<DagNode>> nodeIndex;

    void registerDependency(ReqIdType u, ReqIdType v) {
        if (u > v) std::swap(u, v);
        if (!nodeIndex[u]->isAdjacent(v)) throw BlockError("missing an edge in the dependency graph");
        printf("[%ld->%ld]\n", u, v);
    }
};

} // namespace ascee::runtime
#endif // ASCEE_EXEC_SCHEDULER_H
