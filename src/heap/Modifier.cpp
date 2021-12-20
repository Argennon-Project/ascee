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

#include "Modifier.h"
// #define NDEBUG
#include <cassert>

#define MAX_VERSION 30000

using namespace ascee;
using namespace ascee::runtime::heap;

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
    chunks = &appsAccessMaps[appID];
    currentChunk = nullptr;
}

/// When resizeable is true and newSize > 0 the chunk can only be expanded and the value of
/// newSize indicates the upper bound of the settable size. If resizable == true and newSize <= 0 the chunk is only
/// shrinkable and the magnitude of newSize indicates the lower bound of the chunk's new size.
///
/// When resizeable is false and newSize >= 0 the size of the chunk can only be read but if newSize < 0
/// the size of the chunk is not accessible. (not readable nor writable)
void Modifier::defineChunk(long_id ownerApp, long_id chunkID, Chunk* chunkOnHeap, int32 newSize, bool resizable) {
    std::lock_guard<std::mutex> lock(writeMutex);
    appsAccessMaps[ownerApp].emplace(std::piecewise_construct,
                                     std::forward_as_tuple(chunkID),
                                     std::forward_as_tuple(chunkOnHeap, newSize, resizable));
}

void Modifier::defineAccessBlock(long_id appID, long_id chunkID, int32 offset,
                                 int32 size, bool writable) {
    auto& chunk = appsAccessMaps.at(appID).at(chunkID);
    std::lock_guard<std::mutex> lock(writeMutex);
    chunk.accessTable.emplace(std::piecewise_construct,
                              std::forward_as_tuple(offset),
                              std::forward_as_tuple(chunk.ptr->getContentPointer(offset), size, writable));
}

void Modifier::writeToHeap() {
    if (currentVersion == 0) return;

    for (auto& appMap: appsAccessMaps) {
        for (auto& chunk64Map: appMap.second) {
            auto& chunk = chunk64Map.second;
            auto size = chunk.size.read<int32>(currentVersion);
            if (chunk.size.isWritable()) chunk.ptr->reSize(size);
            if (size > 0) {
                for (auto& block: chunk.accessTable) {
                    try {
                        block.second.wrToHeap(currentVersion);
                    } catch (const std::out_of_range&) {}
                }
            }
        }
    }
}

void Modifier::updateChunkSize(int32 newSize) {
    if (currentChunk->newSize > 0) {
        if (newSize < currentChunk->initialSize || newSize > currentChunk->newSize) {
            throw std::out_of_range("invalid chunk size");
        }
    } else if (newSize > currentChunk->initialSize || newSize < -currentChunk->newSize) {
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

    return versionList.empty() ? heapLocation.get(size) : versionList.back().content;
}

byte* Modifier::AccessBlock::add(int16_t version) {
    assert(version >= 1);
    // checks are ordered for having the best performance on average
    if (!versionList.empty()) {
        auto latestVersion = versionList.back().number;
        assert(latestVersion <= version);
        if (latestVersion == version) return nullptr;
    }

    return versionList.emplace_back(version, size).content;
}

void Modifier::AccessBlock::wrToHeap(int16_t version) {
    syncTo(version);
    if (!versionList.empty()) {
        memcpy(heapLocation.get(size), versionList.back().content, size);
    }
}

Modifier::AccessBlock::AccessBlock(Chunk::Pointer heapLocation,
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
