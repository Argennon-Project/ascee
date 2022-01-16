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

/// works fine even if the graph is not a dag and contains loops
/// RequestSchedulers are created per block
class RequestScheduler {
public:
    ascee::runtime::AppRequest* nextRequest();

    void submitResult(AppRequestIdType reqID, int statusCode);

    void findCollisions(full_id chunkID,
                        const std::vector<int32>& sortedOffsets,
                        const std::vector<BlockAccessInfo>& accessBlocks);

    /// this function is thread-safe as long as all used `id`s are distinct
    auto& requestAt(AppRequestIdType id);

    /// this function is thread-safe as long as all used `id`s are distinct
    void addRequest(AppRequestInfo&& data);

    /// this function should be called after all requests are added. (using addRequest() or requestAt())
    void finalizeRequest(AppRequestIdType id);

    void buildExecDag();

    [[nodiscard]]
    AppRequestInfo::AccessMapType sortAccessBlocks();

    explicit RequestScheduler(int_fast32_t totalRequestCount, asa::ChunkIndex& heapIndex);

    [[nodiscard]]
    ascee::runtime::HeapModifier getModifierFor(AppRequestIdType requestID) const;

private:
    asa::ChunkIndex& heapIndex;
    std::atomic<int_fast32_t> count;
    util::BlockingQueue<DagNode*> zeroQueue;
    std::unique_ptr<std::unique_ptr<DagNode>[]> nodeIndex;
    std::vector<AppRequestInfo::AccessMapType> memoryAccessMaps;


    void registerDependency(AppRequestIdType u, AppRequestIdType v);

    void injectDigest(Digest digest, std::string& httpRequest) {}
};

} // namespace argennon::ave
#endif // ARGENNON_EXEC_SCHEDULER_H
