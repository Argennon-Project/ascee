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

#include "RequestScheduler.h"
#include "ascee/heap/RestrictedModifier.h"


using namespace argennon;
using namespace ave;
using namespace ascee::runtime;
using std::make_unique, std::vector, asa::ChunkIndex;

AppRequest* RequestScheduler::nextRequest() {
    try {
        auto* result = &zeroQueue.blockingDequeue()->getAppRequest();
        zeroQueue.addProducer();
        return result;
    } catch (const std::out_of_range&) {
        if (count != 0) throw BlockError("execution graph is not a dag");
        return nullptr;
    }
}

void RequestScheduler::submitResult(AppRequestIdType reqID, int statusCode) {
    // This function is thread-safe
    auto& reqNode = nodeIndex[reqID];

    if (statusCode > 400 && !reqNode->getAppRequest().attachments.empty()) {
        throw BlockError("block contains a failed fee payment");
    }

    for (const auto id: reqNode->adjacentNodes()) {
        // We assume that adj list of all nodes are checked before, and always we have adjID < nodeIndex.size()
        auto& adjNode = nodeIndex[id];
        if (adjNode->decrementInDegree() == 0) {
            zeroQueue.enqueue(adjNode.get());
        }
    }
    reqNode.reset();
    --count;
    zeroQueue.removeProducer();
}

/// sortedOffsets needs to be a sorted list of offsets, and AccessBlocks are corresponding BlockAccessInfos with
/// those offsets. Access blocks must be non-overlapping.
///
/// we must have offset == -3 for blocks(!writable && size == 0) and offset == -2 for
/// blocks(!writable && size != 0) and offset == -1 for blocks(writable)
///
/// sizeLowerBound is the minimum allowed size of the chunk and it is
/// inclusive. (i.e. it is the mathematical lower bound of chunkSize and we require chunkSize >= sizeLowerBound)
void RequestScheduler::findCollisions(
        full_id chunkID,
        const vector<int32>& sortedOffsets,
        const vector<BlockAccessInfo>& accessBlocks
) {
    bool inSizeWriterList = false;
    int32_fast sizeWritersBegin = 0, sizeWritersEnd = 0;
    int32_fast lowerBound = 0;
    for (int32_fast i = 0; i < accessBlocks.size(); ++i) {
        auto accessType = accessBlocks[i].accessType;
        auto offset = sortedOffsets[i];

        // we can skip non-accessible size blocks because based on their offset they will always be at the start
        // of the list.
        if (offset == -3) continue;

        auto& request = nodeIndex[accessBlocks[i].requestID]->getAppRequest();
        auto end = (offset == -1 || offset == -2) ? 0 : offset + accessBlocks[i].size;

        for (int32_fast j = i + 1; j < accessBlocks.size(); ++j) {
            if (sortedOffsets[j] < end && accessType.collides(accessBlocks[j].accessType)) {
                registerDependency(accessBlocks[i].requestID, accessBlocks[j].requestID);
            }
        }

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

        // offset > upperBound will never occur, since those transactions must not be included in a block.
        if (end > lowerBound) {
            for (int32_fast j = sizeWritersBegin; j < sizeWritersEnd; ++j) {
                auto newSize = accessBlocks[j].size;
                bool collision = newSize > 0 ? offset < newSize : end > -newSize;
                if (collision) registerDependency(accessBlocks[i].requestID, accessBlocks[j].requestID);
            }
        }
    }
}

void RequestScheduler::addRequest(AppRequestInfo&& data) {
    auto id = data.id;
    memoryAccessMaps[id] = std::move(data.memoryAccessMap);
    nodeIndex[id] = std::make_unique<DagNode>(std::move(data), this);
}

auto& RequestScheduler::requestAt(AppRequestIdType id) {
    return nodeIndex[id];
}

RequestScheduler::RequestScheduler(
        int_fast32_t totalRequestCount,
        ChunkIndex& heapIndex
) :
        heapIndex(heapIndex),
        count(totalRequestCount),
        nodeIndex(std::make_unique<std::unique_ptr<DagNode>[]>(totalRequestCount)),
        memoryAccessMaps(totalRequestCount) {}

void RequestScheduler::buildExecDag() {
    for (int i = 0; i < count; ++i) {
        if (nodeIndex[i]->getInDegree() == 0) {
            zeroQueue.enqueue(nodeIndex[i].get());
        } else {
            break;
        }
    }
    if (zeroQueue.isEmpty()) throw BlockError("source node of the execution DAG is missing");
}

AppRequestInfo::AccessMapType RequestScheduler::sortAccessBlocks(int workersCount) {
    return util::mergeAllParallel(std::move(memoryAccessMaps), workersCount);
}

void RequestScheduler::finalizeRequest(AppRequestIdType id) {
    auto& node = nodeIndex[id];
    for (const auto adjID: node->adjacentNodes()) {
        nodeIndex[adjID]->incrementInDegree();
    }

    // add digest to fee payment requests
    for (const auto reqID: node->getAppRequest().attachments) {
        injectDigest(nodeIndex[reqID]->getAppRequest().digest, node->getAppRequest().httpRequest);
    }
}

void RequestScheduler::registerDependency(AppRequestIdType u, AppRequestIdType v) {
    if (!nodeIndex[u]->isAdjacent(v) && !nodeIndex[v]->isAdjacent(u)) {
        throw BlockError("missing {" + std::to_string(u) + "," + std::to_string(v) +
                         "} edge in the dependency graph");
    }
    printf("[%ld--%ld] ", u, v);
}

HeapModifier RequestScheduler::getModifierFor(AppRequestIdType requestID) const {
    return heapIndex.buildModifier(memoryAccessMaps[requestID]);
}

DagNode::DagNode(AppRequestInfo&& data,
                 const RequestScheduler* scheduler) :
        adjList(std::move(data.adjList)),
        request{
                .id = data.id,
                .calledAppID = data.calledAppID,
                .httpRequest = std::move(data.httpRequest),
                .gas = data.gas,
                .modifier = scheduler->getModifierFor(data.id),
                .appTable = AppTable(data.appAccessList),
                .failureManager = FailureManager(
                        std::move(data.stackSizeFailures),
                        std::move(data.cpuTimeFailures)
                ),
                .attachments = std::move(data.attachments),
                .digest = std::move(data.digest)
                // Members are initialized in left-to-right order as they appear in this class's base-specifier list.
        } {}
