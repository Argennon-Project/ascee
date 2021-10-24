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

#define MAX_TRANSIENT_CHUNK_SIZE 8*1024

const int ascee::Heap::Modifier::sizeCell;
const int ascee::Heap::Modifier::maxNewSizeCell;

using namespace ascee;

static
bool isLittleEndian() {
    byte buf[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

    int128 x = *(int128*) buf;
    auto xl = (int64) x;
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

    for (const auto& appAccessList: memAccessList) {
        std_id_t app = appAccessList.appID;
        for (const auto& chunkAccessList: appAccessList.chunks) {
            std_id_t chunk = chunkAccessList.id;

            // chunkID[->positive integer]
            // maxNewSize == -1 means that no positive integer was provided
            int32 maxNewSize = chunkAccessList.maxNewSize;

            Pointer chunkSizePtr = getSizePointer(app, chunk);

            int32 chunkSize;
            if (chunkSizePtr.isNull()) {
                if (maxNewSize > 0) {
                    chunkSizePtr = newChunk(app, chunk, maxNewSize);
                } else {
                    chunkSizePtr = newTransientChunk();
                    // We make sure that a transient chunk is not resizable.
                    // maxNewSize == -2 means that the chunk is transient
                    maxNewSize = -2;
                }
                chunkSize = chunkSizePtr.read<int32>();
            } else {
                chunkSize = chunkSizePtr.read<int32>();
                assert(chunkSize > 0);
                // Currently, we don't allow expanding chunks.
                if (maxNewSize > chunkSize) maxNewSize = chunkSize;
            }

            // maxNewSize == 0 means that the chunk will be deleted (likely)
            if (maxNewSize >= 0) {
                ret->defineAccessBlock(chunkSizePtr, app, chunk, Modifier::sizeCell, 4, true);
                ret->defineAccessBlock(Pointer(nullptr), app, chunk, Modifier::maxNewSizeCell, maxNewSize, false);
            } else {
                ret->defineAccessBlock(chunkSizePtr, app, chunk, Modifier::sizeCell, 4, false);
            }

            for (const auto& blockAccessList: chunkAccessList.accessBlocks) {
                int32 offset = blockAccessList.offset, accessBlockSize = blockAccessList.size;
                bool isWritable = blockAccessList.writable;

                int32 bound;
                Pointer startPtr(nullptr);
                if (chunkSize == 0) {
                    if (maxNewSize == -2) {
                        bound = MAX_TRANSIENT_CHUNK_SIZE;
                    } else {
                        bound = maxNewSize;
                        startPtr = Pointer(chunkSizePtr.heapPtr + 4 + offset);
                    }
                } else {
                    bound = chunkSize;
                    startPtr = Pointer(chunkSizePtr.heapPtr + 4 + offset);
                }

                if (offset + accessBlockSize > bound) throw std::out_of_range("out of chunk");
                ret->defineAccessBlock(startPtr, app, chunk, offset, accessBlockSize, isWritable);
            }
        }
    }

    return ret;
}


Heap::Pointer Heap::getSizePointer(std_id_t appID, std_id_t chunkID) {
    byte* chunkStart;
    try {
        chunkStart = chunkIndex.at(chunkID);
    } catch (const std::out_of_range&) {
        chunkStart = nullptr;
    }
    if (chunkStart == nullptr) {
        // verify the chunk really is absent
    }
    return Heap::Pointer(chunkStart);
}

Heap::Pointer Heap::newTransientChunk() {
    return Heap::Pointer(zero32);
}

Heap::Pointer Heap::newChunk(std_id_t appID, std_id_t id, int32 size) {
    // memory should be zeroed;
    *(std_id_t*) freeArea = appID;
    freeArea += sizeof(std_id_t);

    *(std_id_t*) freeArea = id;
    freeArea += sizeof(std_id_t);
    auto start = freeArea;

    // size of a new chunk should be initialized to zero.
    *(int32*) freeArea = 0;
    freeArea += sizeof(int32) + size;

    return Heap::Pointer(start);
}

void ascee::Heap::saveChunk(Heap::Pointer sizePtr) {
    chunkIndex[*(sizePtr.heapPtr - 8)] = sizePtr.heapPtr;
}

void ascee::Heap::freeChunk(Heap::Pointer sizePtr) {
    chunkIndex.erase(*(sizePtr.heapPtr - 8));
}

