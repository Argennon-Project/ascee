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


using namespace argennon;
using namespace ave;
using namespace ascee::runtime;
using namespace util;
using std::make_unique, std::vector, asa::ChunkIndex;

AppRequest* RequestScheduler::nextRequest() {
    try {
        auto* result = &zeroQueue.blockingDequeue(true)->getAppRequest();
        return result;
    } catch (const std::underflow_error&) {
        if (remaining != 0) throw BlockError("execution graph is not a dag");
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
    --remaining;
    zeroQueue.removeProducer();
}

/// sortedOffsets needs to be a sorted list of offsets, and AccessBlocks are corresponding BlockAccessInfos with
/// those offsets. Access blocks must be non-overlapping.
///
/// we must have offset == -3 for blocks(!writable && !readable) and offset == -2 for
/// blocks(!writable && readable) and offset == -1 for blocks(writable)
///
/// sizeLowerBound is the minimum allowed size of the chunk and it is
/// inclusive. (i.e. it is the mathematical lower bound of chunkSize and we require chunkSize >= sizeLowerBound)
void RequestScheduler::findCollisions(
        full_id chunkID,
        const vector<int32>& sortedOffsets,
        const vector<AccessBlockInfo>& accessBlocks
) {
    int32_fast sizeWritersBegin = 0, sizeWritersEnd = 0;
    bool inSizeWriterList = false;
    int32_fast lowerBound = 0;
    for (int32_fast i = 0; i < accessBlocks.size(); ++i) {
        auto accessType = accessBlocks[i].accessType;
        auto offset = sortedOffsets[i];
        auto reqID = accessBlocks[i].requestID;

        // with this simple if we can skip non-accessible size blocks because based on their offset they will always
        // be at the start of the list.
        if (offset == -3) continue;

        auto end = (offset == -1 || offset == -2) ? 0 : offset + accessBlocks[i].size;

        for (int32_fast j = i + 1; j < accessBlocks.size() && sortedOffsets[j] < end; ++j) {
            if (accessType.collides(accessBlocks[j].accessType)) {
                bool additiveSameBlock = accessType.isAdditive() && accessType == accessBlocks[j].accessType &&
                                         offset == sortedOffsets[j] && accessBlocks[i].size == accessBlocks[j].size;
                if (!additiveSameBlock) registerDependency(reqID, accessBlocks[j].requestID);
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

        // end > upperBound will never occur, since those transactions must not be included in a block.
        if (end > lowerBound) {
            for (int32_fast j = sizeWritersBegin; j < sizeWritersEnd; ++j) {
                if (reqID != accessBlocks[j].requestID) {
                    auto newSize = accessBlocks[j].size;
                    bool collision = newSize > 0 ? offset < newSize : end > -newSize;
                    if (collision) registerDependency(reqID, accessBlocks[j].requestID);
                }
            }
        }
    }
}

void RequestScheduler::addRequest(AppRequestInfo&& data) {
    auto id = data.id;
    memoryAccessMaps[id] = std::move(data.memoryAccessMap);
    nodeIndex[id] = std::make_unique<DagNode>(std::move(data), this);
}

AppRequest* RequestScheduler::requestAt(AppRequestIdType id) {
    return &nodeIndex[id]->getAppRequest();
}

RequestScheduler::RequestScheduler(int32_fast totalRequestCount, ChunkIndex& heapIndex, asa::AppIndex& appIndex) :
        heapIndex(heapIndex),
        appIndex(appIndex),
        remaining(totalRequestCount),
        nodeIndex(std::make_unique<std::unique_ptr<DagNode>[]>(totalRequestCount)),
        memoryAccessMaps(totalRequestCount) {}

void RequestScheduler::buildExecDag() {
    for (int i = 0; i < remaining; ++i) {
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
    printf(" finalized: %ld ", id);
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
    assert(u != v);
    if (!nodeIndex[u]->isAdjacent(v) && !nodeIndex[v]->isAdjacent(u)) {
        throw BlockError("missing {" + std::to_string(u) + "," + std::to_string(v) +
                         "} edge in the dependency graph");
    }
    printf("[%ld--%ld] ", u, v);
}

HeapModifier RequestScheduler::getModifierFor(AppRequestIdType requestID) const {
    return heapIndex.buildModifier(memoryAccessMaps[requestID]);
}

bool RequestScheduler::canMerge(const AccessBlockInfo& left, int32 leftOffset,
                                const AccessBlockInfo& right, int32 rightOffset) {
    return left.accessType == right.accessType &&
           (!left.accessType.isAdditive() || left.accessType.isAdditive() && leftOffset == rightOffset &&
                                             left.size == right.size);
}

RequestScheduler::operator std::string() const {
    std::string result;
    for (int i = 0; i < remaining; ++i) {
        result += std::to_string(nodeIndex[i]->adjacentNodes().size()) + "=";
    }
    return result;
}

AppTable RequestScheduler::getAppTableFor(vector<long_id>&& sortedAppList) const {
    return appIndex.buildAppTable(std::move(sortedAppList));
}

VirtualSignatureManager RequestScheduler::getSigManagerFor(vector<AppRequestInfo::SignedMessage>&& messageList) const {
    if (messageList.empty()) return VirtualSignatureManager({});
    static CryptoSystem crypto;
    vector<VirtualSignatureManager::SignedMessage> result;
    result.reserve(messageList.size());
    for (auto& msg: messageList) {
        // low level access to account data
        bool verifies = false;
        try {
            auto contentPtr = heapIndex.getChunk({arg_app_id_g, {msg.issuerAccount, nonce_chunk_local_id_g}})
                    ->getContentPointer(0, decision_nonce_size_g + util::public_key_size_k).get();
            uint16 decisionNonce = *(uint16*) contentPtr;

            // decisionNonce == 0 means that the owner of the account is an app
            if (decisionNonce != 0) {
                PublicKey pk(contentPtr + decision_nonce_size_g);
                if (crypto.verify(msg.message, msg.signature, pk)) verifies = true;
            }
        } catch (const std::out_of_range&) {}
        result.emplace_back(
                VirtualSignatureManager::SignedMessage{
                        msg.issuerAccount,
                        verifies ? std::move(msg.message) : ""
                });
    }
    return VirtualSignatureManager(std::move(result));
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
                .appTable = scheduler->getAppTableFor(std::move(data.appAccessList)),
                .failureManager = FailureManager(
                        std::move(data.stackSizeFailures),
                        std::move(data.cpuTimeFailures)
                ),
                .attachments = std::move(data.attachments),
                .signatureManager = scheduler->getSigManagerFor(std::move(data.signedMessagesList)),
                .digest = std::move(data.digest)
                // Members are initialized in left-to-right order as they appear in this class's base-specifier list.
        } {}
