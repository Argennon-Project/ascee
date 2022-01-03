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

#include "Modifier.h"
// #define NDEBUG
#include <cassert>
#include <utility>

#define MAX_VERSION 30000

using namespace ascee;
using namespace ascee::runtime::heap;
using std::vector;

int16_t Modifier::saveVersion() {
    if (currentVersion == MAX_VERSION) throw ApplicationError("version limit reached", StatusCode::limit_exceeded);
    return currentVersion++;
}

void Modifier::restoreVersion(int16_t version) {
    // todo: assert?
    if (version >= currentVersion || version < 0) throw std::runtime_error("restoring an invalid version");
    currentVersion = version;
}

void Modifier::loadChunk(short_id chunkID) {
    loadChunk(long_id(chunkID));
}

void Modifier::loadChunk(long_id chunkID) {
    currentChunk = &chunks->at(chunkID);
}

void Modifier::loadContext(long_id appID) {
    // When `appID` does not exist in the map, this function should not throw an exception. Smart contracts do not
    // call this function directly and failing can be problematic for `invoke_dispatcher` function.
    try {
        chunks = &appsAccessMaps.at(appID);
    } catch (const std::out_of_range&) {
        chunks = nullptr;
    }
    currentChunk = nullptr;
}

void Modifier::writeToHeap() {
    if (currentVersion == 0) return;

    for (auto& appMap: appsAccessMaps.getValues()) {
        for (auto& chunk: appMap.getValues()) {
            auto size = chunk.size.read<int32>(currentVersion);
            if (chunk.size.isWritable()) chunk.ptr->setSize(size);
            if (size > 0) {
                for (auto& block: chunk.accessTable.getValues()) {
                    try {
                        block.wrToHeap(currentVersion);
                    } catch (const std::out_of_range&) {}
                }
            }
        }
    }
}

void Modifier::updateChunkSize(int32 newSize) {
    if (currentChunk->newSize > 0) {
        if (newSize < currentChunk->getInitialSize() || newSize > currentChunk->newSize) {
            throw std::out_of_range("invalid chunk size");
        }
    } else if (newSize > currentChunk->getInitialSize() || newSize < -currentChunk->newSize) {
        throw std::out_of_range("invalid chunk size");
    }
    currentChunk->size.write(currentVersion, newSize);
}

int32 Modifier::getChunkSize() {
    if (!currentChunk->size.isWritable() && currentChunk->newSize < 0) {
        throw std::out_of_range("size is not accessible");
    }
    return currentChunk->size.read<int32>(currentVersion);
}

byte* Modifier::AccessBlock::syncTo(int16_t version) {
    while (!versionList.empty() && versionList.back().number > version) {
        versionList.pop_back();
    }

    return versionList.empty() ? heapLocation.get(size) : versionList.back().getContent();
}

byte* Modifier::AccessBlock::add(int16_t version) {
    assert(version >= 1);
    // checks are ordered for having the best performance on average
    if (!versionList.empty()) {
        auto latestVersion = versionList.back().number;
        assert(latestVersion <= version);
        if (latestVersion == version) return nullptr;
    }

    return versionList.emplace_back(version, size).getContent();
}

void Modifier::AccessBlock::wrToHeap(int16_t version) {
    syncTo(version);
    if (!versionList.empty()) {
        memcpy(heapLocation.get(size), versionList.back().getContent(), size);
    }
}

Modifier::AccessBlock::AccessBlock(const Chunk::Pointer& heapLocation,
                                   int32 size,
                                   bool writable) : heapLocation(heapLocation),
                                                    size(size),
                                                    writable(writable) {}

void Modifier::AccessBlock::prepareToWrite(int16_t version, int writeSize) {
    if (writeSize > size) throw std::out_of_range("write size");
    if (!writable) throw std::out_of_range("block is not writable");
    auto oldContent = syncTo(version);
    auto newContent = add(version);
    if (size != writeSize && newContent != nullptr) {
        memcpy(newContent + writeSize, oldContent + writeSize, size - writeSize);
    }
    // Here we don't return the pointer to `content`. It would be better if the caller always finds the end of the
    // version list using back().content. That way content of the heap would not be modified accidentally.
}

Modifier::ChunkInfo::ChunkInfo(
        Chunk* chunk, int32 newSize, bool resizable,
        std::vector<int32> sortedAccessedOffsets,
        const std::vector<BlockAccessInfo>& accessInfoList
) :
// todo: explain why we use initialSize member
        size(
                Chunk::Pointer((byte*) (&initialSize)),
                sizeof(int32),
                resizable
        ),
        newSize(newSize),
        ptr(chunk),
        accessTable(std::move(sortedAccessedOffsets), toBlocks(chunk, sortedAccessedOffsets, accessInfoList)) {
    // Here resize must not perform any reallocation. As long as sizeof(int32) is less than the capacity of the chunk
    // this should not happen.
    initialSize = chunk->getsize();
}

vector<Modifier::AccessBlock> Modifier::ChunkInfo::toBlocks(
        Chunk* chunk,
        const vector<int32>& offsets,
        const vector<BlockAccessInfo>& accessInfoList
) {
    vector<AccessBlock> blocks;
    blocks.reserve(offsets.size());
    for (int i = 0; i < offsets.size(); ++i) {
        if (accessInfoList[i].writable && !chunk->isWritable()) {
            throw BlockError("trying to modify a readonly chunk");
        }
        blocks.emplace_back(
                chunk->getContentPointer(offsets[i]),
                accessInfoList[i].size,
                accessInfoList[i].writable
        );
    }
    return blocks;
}
