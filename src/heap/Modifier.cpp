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

#include "Heap.h"
// #define NDEBUG
#include <cassert>

#define MAX_VERSION 30000

using namespace ascee;
using namespace ascee::runtime;

int16_t Heap::Modifier::saveVersion() {
    if (currentVersion == MAX_VERSION) throw AsceeException("version limit reached", StatusCode::limit_exceeded);
    return currentVersion++;
}

void Heap::Modifier::restoreVersion(int16_t version) {
    // todo: assert?
    if (version >= currentVersion || version < 0) throw std::runtime_error("restoring an invalid version");
    currentVersion = version;
}

void Heap::Modifier::loadChunk(short_id chunkID) {
    loadChunk(long_id(chunkID));
}

void Heap::Modifier::loadChunk(long_id chunkID) {
    currentChunk = &chunks->at(chunkID);
    accessTable = &currentChunk->accessTable;
}

void Heap::Modifier::loadContext(long_id appID) {
    // When `appID` does not exist in the map, this function should not throw an exception. Smart contracts do not
    // call this function directly and failing can be problematic for `invoke_dispatcher` function.
    chunks = &appsAccessMaps[appID];
    accessTable = nullptr;
    currentChunk = nullptr;
}

void Heap::Modifier::defineChunk(long_id ownerApp, long_id chunkID, Chunk::Pointer sizePtr, int32 maxSize) {
    bool inserted = appsAccessMaps[ownerApp].emplace(std::piecewise_construct,
                                                     std::forward_as_tuple(chunkID),
                                                     std::forward_as_tuple(sizePtr, maxSize)).second;
    if (!inserted) throw std::invalid_argument("chunk already exists");
}

void Heap::Modifier::defineAccessBlock(Chunk::Pointer heapLocation,
                                       long_id app, long_id chunk, int32 offset,
                                       int32 size, bool writable) {
    bool inserted = appsAccessMaps.at(app).at(chunk).accessTable.emplace(std::piecewise_construct,
                                                                         std::forward_as_tuple(offset),
                                                                         std::forward_as_tuple(heapLocation, size,
                                                                                               writable)).second;
    if (!inserted) throw std::invalid_argument("block already exists");
}

void Heap::Modifier::writeToHeap() {
    if (currentVersion == 0) return;

    for (auto& appMap: appsAccessMaps) {
        for (auto& chunk64Map: appMap.second) {
            auto& chunk = chunk64Map.second;
            auto size = chunk.size.read<int32>(currentVersion);

            chunk.size.wrToHeap(currentVersion);
            if (size > 0) {
                for (auto& block: chunk.accessTable) {
                    block.second.wrToHeap(currentVersion);
                }
            } else {
                parent->freeChunk(appMap.first, chunk64Map.first);
            }
        }
    }

    parent = nullptr;
}

void Heap::Modifier::updateChunkSize(int32 newSize) {
    if (newSize > currentChunk->maxSize || newSize < 0) {
        throw std::out_of_range("invalid chunk size");
    }
    currentChunk->size.write(currentVersion, newSize);
}

int32 Heap::Modifier::getChunkSize() {
    return currentChunk->size.read<int32>(currentVersion);
}

byte* Heap::Modifier::AccessBlock::syncTo(int16_t version) {
    while (!versionList.empty() && versionList.back().number > version) {
        versionList.pop_back();
    }

    return versionList.empty() ? heapLocation.get() : versionList.back().content;
}

byte* Heap::Modifier::AccessBlock::add(int16_t version) {
    assert(version >= 1);
    // checks are ordered for having the best performance on average
    if (!versionList.empty()) {
        auto latestVersion = versionList.back().number;
        assert(latestVersion <= version);
        if (latestVersion == version) return nullptr;
    }

    return versionList.emplace_back(version, size).content;
}

void Heap::Modifier::AccessBlock::wrToHeap(int16_t version) {
    syncTo(version);
    if (!versionList.empty()) {
        heapLocation.writeBlock(versionList.back().content, size);
    }
}

Heap::Modifier::AccessBlock::AccessBlock(Chunk::Pointer heapLocation,
                                         int32 size,
                                         bool writable) : heapLocation(heapLocation),
                                                          size(size),
                                                          writable(writable) {}

void Heap::Modifier::AccessBlock::prepareToWrite(int16_t version, int writeSize) {
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
