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

using namespace ascee::runtime;

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
    auto& txNode = nodeIndex.at(result.txID);
    for (const auto& adjacent: txNode->adjacentNodes()) {
        if (adjacent->DecrementInDegree() == 0) {
            zeroQueue.enqueue(adjacent);
        }
    }
    // We don't use nodeIndex.erase(...) because if a rehash happens that would not be thread-safe.
    txNode.reset();
    --count;
    zeroQueue.removeProducer();
}

void RequestScheduler::addMemoryAccessList(ascee::long_id appID, ascee::long_id chunkID,
                                           const std::vector<AccessBlock>& sortedAccessBlocks) {
    auto chunkPtr = heap.getChunk(appID, chunkID);

    for (int i = 0; i < sortedAccessBlocks.size(); ++i) {
        auto& request = nodeIndex[sortedAccessBlocks[i].txID]->getAppRequest();
        auto end = sortedAccessBlocks[i].offset + sortedAccessBlocks[i].size;
        auto writable = sortedAccessBlocks[i].writable;
        auto offset = sortedAccessBlocks[i].offset;

        if (offset == -1) {
            auto newSize = sortedAccessBlocks[i].size;
            request.modifier.defineChunk(appID, chunkID, chunkPtr, newSize, writable);
            end = 0;
        } else {
            request.modifier.defineAccessBlock(appID, chunkID, offset,
                                               sortedAccessBlocks[i].size, writable);
        }

        for (int j = i + 1; j < sortedAccessBlocks.size(); ++j) {
            if (sortedAccessBlocks[j].offset < end && (writable || sortedAccessBlocks[j].writable)) {
                graph.registerDependency(sortedAccessBlocks[i].txID, sortedAccessBlocks[j].txID);
            }
        }
    }

}
