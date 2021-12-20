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

#ifndef ASCEE_CACHE_H
#define ASCEE_CACHE_H


#include <argc/types.h>
#include <unordered_map>
#include <vector>
#include <util/IdentifierTrie.h>
#include <mutex>
#include "Chunk.h"
#include "Page.h"

namespace ascee::runtime::heap {

struct AppMemAccess {
    struct Block {
        int32 offset;
        int32 size;
    };

    struct Chunk {
        long_id id;
        int32 maxNewSize;
        std::vector<Block> writableBlocks;
        std::vector<Block> readOnlyBlocks;
    };

    long_id appID;
    std::vector<Chunk> chunks;
};

//TODO: Heap must be signal-safe but it does not need to be thread-safe
class Cache {
public:
    Cache();

    void freeChunk(long_id appID, long_id id);

    Chunk* getChunk(long_id appID, long_id chunkID);

    void cacheBlockData(std::vector<full_id> requiredPages, std::vector<full_id> newChunks) {
        for (const auto& newChunkID: newChunks) {
            newChunk(newChunkID, 16);
        }
    }

private:
    std::unordered_map<int128, Chunk*> chunkIndex;
    std::unordered_map<int128, Page> pageCache;

    Chunk* newChunk(full_id id, int32 size);
};

} // namespace ascee::runtime::heap

#endif // ASCEE_CACHE_H
