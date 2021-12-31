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

#include "RequestScheduler.h"
#include "loader/BlockLoader.h"

using namespace ascee::runtime;
using std::make_unique, std::vector;

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

void RequestScheduler::submitResult(const AppResponse& result) {
    // This function is thread-safe
    auto& txNode = nodeIndex[result.reqID];
    for (const auto id: txNode->adjacentNodes()) {
        // We assume that adj list of all nodes are checked before, and always we have adjID < nodeIndex.size()
        auto& adjNode = nodeIndex[id];
        if (adjNode->decrementInDegree() == 0) {
            zeroQueue.enqueue(adjNode.get());
        }
    }
    txNode.reset();
    --count;
    zeroQueue.removeProducer();
}

void RequestScheduler::findCollisions(ascee::long_id appID, ascee::long_id chunkID) {
    auto blocks = sortedAccessBlocks->at(appID).at(chunkID).getConstValues();
    for (int i = 0; i < blocks.size(); ++i) {
        auto& request = nodeIndex[blocks[i].requestID]->getAppRequest();
        auto writable = blocks[i].writable;
        auto offset = blocks[i].offset;
        auto end = (offset == -1) ? 0 : blocks[i].offset + blocks[i].size;

        for (int j = i + 1; j < blocks.size(); ++j) {
            if (blocks[j].offset < end && (writable || blocks[j].writable)) {
                registerDependency(blocks[i].requestID, blocks[j].requestID);
            }
        }
    }
}

void RequestScheduler::addRequest(AppRequest::IdType id, AppRequestRawData&& data) {
    memAccessMaps[id] = std::move(data.memAccessMap);
    nodeIndex[id] = std::make_unique<DagNode>(std::move(data), this);
}

auto& RequestScheduler::requestAt(AppRequest::IdType id) {
    return nodeIndex[id];
}

RequestScheduler::RequestScheduler(int_fast32_t totalRequestCount, heap::PageCache::ChunkIndex& heapIndex) :
        heapIndex(heapIndex),
        count(totalRequestCount),
        nodeIndex(std::make_unique<std::unique_ptr<DagNode>[]>(totalRequestCount)),
        memAccessMaps(totalRequestCount) {}

heap::Modifier RequestScheduler::buildModifier(const AppRequestRawData::MemAccessMapType& rawAccessMap) const {
    std::vector<heap::Modifier::ChunkMap64> chunkMapList;
    chunkMapList.reserve(rawAccessMap.size());
    for (int i = 0; i < rawAccessMap.size(); ++i) {
        auto appID = rawAccessMap.getKeys()[i];
        std::vector<heap::Modifier::ChunkInfo> chunkInfoList;
        auto& chunkMap = rawAccessMap.getConstValues()[i];
        chunkInfoList.reserve(chunkMap.size());
        for (int j = 0; j < chunkMap.size(); ++j) {
            auto chunkID = chunkMap.getKeys()[j];
            auto chunkPtr = heapIndex.getChunk(full_id(appID, chunkID));

            chunkInfoList.emplace_back(
                    chunkPtr,
                    chunkMap.getConstValues()[j].getConstValues()[0].size,
                    chunkMap.getConstValues()[j].getConstValues()[0].writable,
                    chunkMap.getConstValues()[j].getKeys(),
                    chunkMap.getConstValues()[j].getConstValues()
            );
        }
        chunkMapList.emplace_back(chunkMap.getKeys(), std::move(chunkInfoList));
    }
    return {rawAccessMap.getKeys(), std::move(chunkMapList)};
}

void RequestScheduler::buildExecDag() {
    for (int i = 0; i < count; ++i) {
        if (nodeIndex[i]->getInDegree() == 0) {
            zeroQueue.enqueue(nodeIndex[i].get());
        }
    }
}

void RequestScheduler::sortAccessBlocks() {
    sortedAccessBlocks = make_unique<AppRequestRawData::MemAccessMapType>(
            util::mergeAllParallel(std::move(memAccessMaps))
    );
    memAccessMaps.clear();
}

void RequestScheduler::finalizeRequest(AppRequest::IdType id) {
    auto& node = nodeIndex[id];
    for (const auto adjID: node->adjacentNodes()) {
        nodeIndex[adjID]->incrementInDegree();
    }

    // Correct fee payment requests
    for (const auto reqID: node->getAppRequest().attachments) {
        injectDigest(nodeIndex[reqID]->getAppRequest().digest, node->getAppRequest().httpRequest);
    }

    // we don't need attachments anymore
    node->getAppRequest().attachments.clear();
}

void RequestScheduler::registerDependency(AppRequest::IdType u, AppRequest::IdType v) {
    if (u > v) std::swap(u, v);
    if (!nodeIndex[u]->isAdjacent(v)) throw BlockError("missing an edge in the dependency graph");
    printf("[%ld->%ld]\n", u, v);
}

DagNode::DagNode(AppRequestRawData&& data, const RequestScheduler* scheduler) :
        adjList(std::move(data.adjList)),
        request{
                .id = data.id,
                .calledAppID = data.calledAppID,
                .httpRequest = std::move(data.httpRequest),
                .gas = data.gas,
                .modifier = scheduler->buildModifierFor(data.id),
                .appTable = AppLoader::global->createAppTable(data.appAccessList),
                .failureManager = FailureManager(
                        std::move(data.stackSizeFailures),
                        std::move(data.cpuTimeFailures)
                ),
                .attachments = std::move(data.attachments),
                .digest = std::move(data.digest)
                // Members are initialized in left-to-right order as they appear in this class's base-specifier list.
        } {}
