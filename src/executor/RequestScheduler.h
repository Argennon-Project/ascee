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
#include "heap/Cache.h"
#include "heap/Modifier.h"
#include "FailureManager.h"
#include <atomic>
#include <utility>

namespace ascee::runtime {

struct AppRequest {
    typedef int_fast32_t IdType;
    IdType id;
    long_id calledAppID;
    std::string httpRequest;
    int_fast32_t gas;
    heap::Modifier modifier;
    std::unordered_map<long_id, dispatcher_ptr> appTable;
    FailureManager failureManager;
/*
    AppRequest(IdType id, long_id calledAppId, std::string request, int_fast32_t gas) : id(id),
                                                                                        calledAppID(calledAppId),
                                                                                        request(std::move(request)),
                                                                                        gas(gas) {}*/
};

struct AppResponse {
    AppRequest::IdType txID;
    int statusCode;
    std::string response;
};

class DagNode {
public:
    int DecrementInDegree() {
        std::lock_guard<std::mutex> lock(degreeMutex);
        assert(inDegree > 0);
        return --inDegree;
    }

    AppRequest& getAppRequest() {
        return tx;
    }

    const std::vector<DagNode*>& adjacentNodes() {
        return adjList;
    }

    DagNode(AppRequest::IdType id) : tx{.id = id} {}

private:
    AppRequest tx;
    std::vector<DagNode*> adjList;
    int inDegree = 0;
    std::mutex degreeMutex;
};

/// works fine even if the graph is not a dag and contains loops
class RequestScheduler {
public:
    struct AccessBlock {
        int32 offset;
        int32 size;
        AppRequest::IdType txID;
        bool writable;
    };

    class DependencyGraph {
    public:
        void registerDependency(AppRequest::IdType u, AppRequest::IdType v) {
            printf("[%ld->%ld]\n", u, v);
        }
    };

    AppRequest* nextRequest();;

    void submitResult(const AppResponse& result);;

    void addMemoryAccessList(long_id appID, long_id chunkID, const std::vector<AccessBlock>& sortedAccessBlocks);

    void addRequest(AppRequest::IdType id) {
        nodeIndex.at(id) = std::make_unique<DagNode>(id);
    }

    explicit RequestScheduler(int_fast32_t totalCount, heap::Cache& heap) : heap(heap), count(totalCount),
                                                                            nodeIndex(totalCount) {
        //todo: change
    }

//todo: change this
    DependencyGraph graph;
private:
    heap::Cache& heap;
    std::atomic<int_fast32_t> count;
    BlockingQueue<DagNode*> zeroQueue;
    std::vector<std::unique_ptr<DagNode>> nodeIndex;
};

} // namespace ascee::runtime
#endif // ASCEE_EXEC_SCHEDULER_H
