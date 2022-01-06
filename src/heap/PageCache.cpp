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

#include "PageCache.h"

using namespace ascee;
using namespace ascee::runtime::heap;

static
bool isLittleEndian() {
    byte buf[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    int128 x = *(int128*) buf;
    auto xl = int64(x);
    auto xh = int64(x >> 64);
    if (xl != 0x706050403020100) return false;
    if (xh != 0xf0e0d0c0b0a0908) return false;
    return true;
}

PageCache::PageCache(runtime::PageLoader& loader) : loader(loader) {
    if (!isLittleEndian()) throw std::runtime_error("platform not supported");
}

Modifier PageCache::ChunkIndex::buildModifier(const AppRequestRawData::AccessMapType& rawAccessMap) {
    std::vector<heap::Modifier::ChunkMap64> chunkMapList;
    chunkMapList.reserve(rawAccessMap.size());
    for (long i = 0; i < rawAccessMap.size(); ++i) {
        auto appID = rawAccessMap.getKeys()[i];
        auto& chunkMap = rawAccessMap.getValues()[i];
        std::vector<heap::Modifier::ChunkInfo> chunkInfoList;
        chunkInfoList.reserve(chunkMap.size());
        for (long j = 0; j < chunkMap.size(); ++j) {
            auto chunkID = chunkMap.getKeys()[j];
            // When the chunk is not found getChunk throws a BlockError exception.
            auto* chunkPtr = getChunk(full_id(appID, chunkID));
            auto chunkNewSize = chunkMap.getValues()[j].getValues()[0].size;
            bool resizable = chunkMap.getValues()[j].getValues()[0].writable;

            if (resizable) {
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
                    chunkNewSize,
                    resizable,
                    chunkMap.getValues()[j].getKeys(),
                    chunkMap.getValues()[j].getValues()
            );
        }
        chunkMapList.emplace_back(chunkMap.getKeys(), std::move(chunkInfoList));
    }
    return {rawAccessMap.getKeys(), std::move(chunkMapList)};
}

PageCache::ChunkIndex::ChunkIndex(
        PageCache& parent,
        const runtime::BlockHeader& block,
        std::vector<PageAccessInfo>&& requiredPages,
        util::FixedOrderedMap<full_id, ChunkSizeBounds> chunkBounds
) : parent(parent), pageAccessList(std::move(requiredPages)), sizeBoundsInfo(std::move(chunkBounds)) {
    chunkIndex.reserve(this->pageAccessList.size() * pageAvgLoadFactor / 100);
    parent.loader.setCurrentBlock(block);
    for (const auto& page: this->pageAccessList) {
        // note that since we've used [] if the page doesn't exist in the cache an empty page will be created.
        parent.loader.preparePage(page.id, parent.cache[page.id]);
    }

    for (const auto& page: this->pageAccessList) {
        indexPage(page);
    }

    // after indexing requiredPages all chunks are added to chunkIndex, including accessed non-existent chunks.
    // getChunk() will throw the right exception when chunk is not found.
    for (int i = 0; i < sizeBoundsInfo.size(); ++i) {
        getChunk(sizeBoundsInfo.getKeys()[i])->reserveSpace(sizeBoundsInfo.getValues()[i].sizeUpperBound);
    }
}
