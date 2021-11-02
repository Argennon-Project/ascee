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

#define MAX_TRANSIENT_CHUNK_SIZE 8*1024

#define TRANSIENT_ID 0
const int ascee::Heap::Modifier::sizeCell;
const int ascee::Heap::Modifier::maxNewSizeCell;

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

            Chunk* chunk = getChunk(appID, chunkID);

            int32 chunkSize;
            if (chunk == nullptr) {
                if (maxNewSize > 0) {
                    chunk = newChunk(appID, chunkID, maxNewSize);
                } else if (maxNewSize == 0) {
                    chunk = Chunk::transient;
                    // We make sure that a transient chunk is not resizable.
                    // maxNewSize == -2 means that the chunk is transient
                    maxNewSize = -2;
                } else {
                    throw std::runtime_error("chunk not found");
                }
                chunkSize = chunk->getsize();
            } else {
                chunkSize = chunk->getsize();
                assert(chunkSize > 0);
                // Currently, we don't allow expanding chunks.
                if (maxNewSize > chunkSize) maxNewSize = chunkSize;
            }

            // maxNewSize == 0 means that the chunk will be deleted (likely)
            if (maxNewSize >= 0) {
                // resizable chunk
                ret->defineAccessBlock(chunk->getSizePointer(), appID, chunkID, Modifier::sizeCell, sizeof(chunkSize),
                                       true);
                ret->defineAccessBlock(Chunk::null, appID, chunkID, Modifier::maxNewSizeCell, maxNewSize, false);
            } else {
                // non-resizable chunk
                ret->defineAccessBlock(chunk->getSizePointer(), appID, chunkID, Modifier::sizeCell, sizeof(chunkSize),
                                       false);
            }

            for (const auto& blockAccessList: chunkAccessList.accessBlocks) {
                int32 offset = blockAccessList.offset, accessBlockSize = blockAccessList.size;
                bool isWritable = blockAccessList.writable;

                int32 bound;
                if (chunkSize == 0) {
                    if (chunk->isTransient()) {
                        bound = MAX_TRANSIENT_CHUNK_SIZE;
                    } else {
                        bound = maxNewSize;
                    }
                } else {
                    bound = chunkSize;
                }

                if (offset + accessBlockSize > bound) throw std::out_of_range("out of chunk");
                ret->defineAccessBlock(chunk->getContentPointer(offset), appID, chunkID, offset, accessBlockSize,
                                       isWritable);
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
    auto* result = new Chunk(size);
    tempArea.emplace(full_id_t(appID, id), result);
    return result;
}

void Heap::saveChunk(std_id_t appID, std_id_t id) {
    auto newID = full_id_t(appID, id);
    Chunk* newChunk = tempArea.at(full_id_t(appID, id)).release();
    pageCache[newID].addNewChunk(appID, id, newChunk);
    chunkIndex[newID] = newChunk;
}

void Heap::freeChunk(std_id_t appID, std_id_t id) {
    chunkIndex[full_id_t(appID, id)] = nullptr;
}

