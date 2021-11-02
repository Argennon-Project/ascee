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

/*
uint8_t HeapModifier::loadByte(int32_t offset) {
    return accessTable.at(offset & ~0b111).heapLocation[offset & 0b111];
}*/

int16_t Heap::Modifier::saveVersion() {
    if (currentVersion == MAX_VERSION) throw std::out_of_range("version limit reached");
    return currentVersion++;
}

void Heap::Modifier::restoreVersion(int16_t version) {
    if (version >= currentVersion || version < 0) throw std::runtime_error("restoring an invalid version");
    currentVersion = version;
}

void Heap::Modifier::loadChunk(short_id_t chunkID) {
    loadChunk(std_id_t(chunkID));
}

void Heap::Modifier::loadChunk(std_id_t chunkID) {
    accessTable = &chunks->at(chunkID);
}

void Heap::Modifier::loadContext(std_id_t appID) {
    // When `appID` does not exist in the map, this function should not throw an exception. Smart contracts do not
    // call this function directly and failing can be problematic for `invoke_dispatcher` function.
    chunks = &appsAccessMaps[appID];
    accessTable = nullptr;
}

void Heap::Modifier::defineAccessBlock(Chunk::Pointer heapLocation,
                                       std_id_t app, std_id_t chunk, int32 offset,
                                       int32 size, bool writable) {
    appsAccessMaps[app][chunk].emplace(std::piecewise_construct,
                                       std::forward_as_tuple(offset),
                                       std::forward_as_tuple(heapLocation, size, writable));
}

void Heap::Modifier::writeToHeap() {
    if (currentVersion == 0) return;

    for (auto& appMap: appsAccessMaps) {
        for (auto& chunk64Map: appMap.second) {
            auto& table = chunk64Map.second;
            auto size = table.at(sizeCell).read<int32>(currentVersion);
            try {
                auto maxNewSize = table.at(maxNewSizeCell).getSize();
                size = std::min(size, maxNewSize);
                table.at(sizeCell).write(currentVersion, size);
                if (size > 0) {
                    parent->saveChunk(appMap.first, chunk64Map.first);
                } else {
                    parent->freeChunk(appMap.first, chunk64Map.first);
                }
            } catch (const std::out_of_range&) {}

            if (size > 0) {
                for (auto& block: table) {
                    block.second.wrToHeap(currentVersion);
                }
            }
        }
    }
    parent = nullptr;
}

void Heap::Modifier::updateChunkSize(int32 newSize) {
    // We don't need to check the new size here. The size will be corrected based on the maximum size defined
    // at maxSizeReservedOffset later. If the chunk is not resizable this location is not writable and no
    // checks are needed the call will fail itself.
    accessTable->at(sizeCell).write<int32>(currentVersion, newSize);
}

int32 Heap::Modifier::getChunkSize() {
    return accessTable->at(sizeCell).read<int32>(currentVersion);
}

void Heap::Modifier::AccessBlock::syncTo(int16_t version) {
    while (!versionList.empty() && versionList.back().number > version) {
        versionList.pop_back();
    }
}

bool Heap::Modifier::AccessBlock::add(int16_t version) {
    assert(version >= 1);
    // checks are ordered for having the best performance on average
    if (!versionList.empty()) {
        auto latestVersion = versionList.back().number;
        assert(latestVersion <= version);
        if (latestVersion == version) return false;
    }

    versionList.emplace_back(version, size);
    return true;
}

void Heap::Modifier::AccessBlock::wrToHeap(int16_t version) {
    syncTo(version);
    if (!versionList.empty()) {
        heapLocation.writeBlock(versionList.back().content, size);
    }
}

Heap::Modifier::AccessBlock::AccessBlock(Chunk::Pointer heapLocation, int32 size, bool writable) : heapLocation(
        heapLocation),
                                                                                                   size(size),
                                                                                                   writable(writable) {}
