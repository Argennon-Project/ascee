// Copyright (c) 2022-2022 aybehrouz <behrouz_ayati@yahoo.com>. All rights
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

#include "ChunkIndex.h"

using namespace argennon;
using namespace asa;
using namespace ascee::runtime;
using std::vector, std::pair;

ChunkIndex::ChunkIndex(
        vector<pair<full_id, Page*>>&& requiredPages,
        util::FixedOrderedMap<full_id, ChunkBoundsInfo>&& chunkBounds,
        int32_fast numOfChunks
) : pageList(std::move(requiredPages)), sizeBoundsInfo(std::move(chunkBounds)) {

    chunkIndex.reserve(numOfChunks);
    for (const auto& pageInfo: this->pageList) {
        indexPage(pageInfo);
    }

    // after indexing requiredPages all chunks are added to chunkIndex, including accessed non-existent chunks.
    // getChunk() will throw a BlockError exception correctly when the chunk is not found.
    for (int i = 0; i < sizeBoundsInfo.size(); ++i) {
        getChunk(sizeBoundsInfo.getKeys()[i])->reserveSpace(sizeBoundsInfo.getValues()[i].sizeUpperBound);
    }
}

HeapModifier ChunkIndex::buildModifier(const AppRequestInfo::AccessMapType& rawAccessMap) {
    vector<HeapModifier::ChunkMap64> chunkMapList;
    chunkMapList.reserve(rawAccessMap.size());
    for (long i = 0; i < rawAccessMap.size(); ++i) {
        auto appID = rawAccessMap.getKeys()[i];
        auto& chunkMap = rawAccessMap.getValues()[i];
        vector<HeapModifier::ChunkInfo> chunkInfoList;
        chunkInfoList.reserve(chunkMap.size());
        for (long j = 0; j < chunkMap.size(); ++j) {
            auto chunkID = chunkMap.getKeys()[j];
            // When the chunk is not found getChunk throws a BlockError exception.
            auto* chunkPtr = getChunk(full_id(appID, chunkID));
            auto offset = chunkMap.getValues()[j].getKeys()[0];
            auto chunkNewSize = chunkMap.getValues()[j].getValues()[0].size;

            using Resizing = HeapModifier::ChunkInfo::ResizingType;

            Resizing resizingType = Resizing::non_accessible;
            if (offset == -2) resizingType = Resizing::read_only;
            else if (offset == -1 && chunkNewSize > 0) resizingType = Resizing::expandable;
            else if (offset == -1 && chunkNewSize <= 0) {
                resizingType = Resizing::shrinkable;
                chunkNewSize = -chunkNewSize;
            }

            // if chunk is resizable we check the validity of the proposed chunk bounds
            if (offset == -1) {
                try {
                    auto& chunkBounds = sizeBoundsInfo.at(full_id(appID, chunkID));
                    if (chunkPtr->getsize() < chunkBounds.sizeLowerBound ||
                        chunkPtr->getsize() > chunkBounds.sizeUpperBound ||
                        chunkNewSize > 0 && chunkNewSize > chunkBounds.sizeUpperBound ||
                        chunkNewSize <= 0 && -chunkNewSize < chunkBounds.sizeLowerBound) {
                        throw BlockError("invalid sizeBounds for chunk [" +
                                         std::to_string(appID) + "::" + std::to_string(chunkID) + "]");
                    }
                } catch (const std::out_of_range&) {
                    throw BlockError("missing sizeBounds for chunk ["
                                     + std::to_string(appID) + "::" + std::to_string(chunkID) + "]");
                }
            }

            chunkInfoList.emplace_back(
                    chunkPtr,
                    resizingType,
                    chunkNewSize,
                    chunkMap.getValues()[j].getKeys(),
                    chunkMap.getValues()[j].getValues()
            );
        }
        chunkMapList.emplace_back(chunkMap.getKeys(), std::move(chunkInfoList));
    }
    return {rawAccessMap.getKeys(), std::move(chunkMapList)};
}

Chunk* ChunkIndex::getChunk(full_id id) {
    try {
        return chunkIndex.at(id);
    } catch (const std::out_of_range&) {
        throw BlockError("missing proof of non-existence");
    }
}

void ChunkIndex::indexPage(const pair<full_id, Page*>& pageInfo) {
    chunkIndex.emplace(pageInfo.first, pageInfo.second->getNative());

    for (auto& migrant: pageInfo.second->getMigrants()) {
        chunkIndex.emplace(migrant.first, migrant.second.get());
    }
}

int32_fast ChunkIndex::getSizeLowerBound(full_id chunkID) {
    try {
        return sizeBoundsInfo.at(chunkID).sizeLowerBound;
    } catch (const std::out_of_range&) {
        throw BlockError("missing chunk size bounds");
    }
}
