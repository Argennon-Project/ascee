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

#include <cassert>
#include "Heap.h"
#include "Chunk.h"
#include "Page.h"

#define MAX_CHUNK_SIZE 32*1024

using namespace ascee;

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

Heap::Heap() {
    if (!isLittleEndian()) throw std::runtime_error("platform not supported");
}

Heap::Modifier* Heap::initSession(std_id_t calledApp) {
    return new Modifier(this);
}

Heap::Modifier* Heap::initSession(const std::vector<AppMemAccess>& memAccessList) {
    auto* ret = new Modifier(this);
    tempArea.clear();

    for (const auto& appAccessList: memAccessList) {
        std_id_t appID = appAccessList.appID;
        for (const auto& chunkAccessList: appAccessList.chunks) {
            std_id_t chunkID = chunkAccessList.id;

            // chunkID[->positive integer]
            // maxNewSize == -1 means that no positive integer was provided
            int32 maxNewSize = chunkAccessList.maxNewSize;
            if (maxNewSize > MAX_CHUNK_SIZE) throw std::invalid_argument("max chunk size exceeded");

            Chunk* chunk = getChunk(appID, chunkID);

            int32 chunkSize;
            if (chunk == nullptr) {
                if (maxNewSize >= 0) {
                    chunk = newChunk(appID, chunkID, maxNewSize);
                    chunkSize = 0;
                } else {
                    throw std::out_of_range("chunk not found");
                }
            } else {
                chunkSize = chunk->getsize();
                assert(chunkSize > 0);
                // We inform the chunk to allocate more space
                if (maxNewSize > chunkSize) {
                    chunk->expandSpace(maxNewSize - chunkSize);
                }
            }

            // maxNewSize == 0 means that the chunk will be deleted (likely)
            // maxNewSize >= 0 means that chunk is resizable
            // maxNewSize == -1 means that chunk is NOT resizable
            // maxNewSize < -1 doesn't happen
            ret->defineChunk(appID, chunkID, chunk->getSizePointer(), maxNewSize);

            for (const auto& blockAccessList: chunkAccessList.accessBlocks) {
                int32 offset = blockAccessList.offset, accessBlockSize = blockAccessList.size;
                bool isWritable = blockAccessList.writable;

                int32 bound = std::max(chunkSize, maxNewSize);;

                if (offset + accessBlockSize > bound) throw std::out_of_range("out of chunk");
                ret->defineAccessBlock(chunk->getContentPointer(offset), appID, chunkID, offset,
                                       accessBlockSize, isWritable);
            }
        }
    }

    return ret;
}

Chunk* Heap::getChunk(std_id_t appID, std_id_t chunkID) {
    try {
        return chunkIndex.at(full_id_t(appID, chunkID));
    } catch (const std::out_of_range&) {
        throw std::runtime_error("missing proof of non-existence");
    }
}

Chunk* Heap::newChunk(std_id_t appID, std_id_t id, int32 size) {
    auto* newChunk = new Chunk(size);
    auto newID = full_id_t(appID, id);
    pageCache.emplace(newID, newChunk);
    chunkIndex[newID] = newChunk;
    return newChunk;
}

void Heap::freeChunk(std_id_t appID, std_id_t id) {
    chunkIndex[full_id_t(appID, id)] = nullptr;
}

