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

#include "RestrictedModifier.h"
#include "argc/types.h"
#include <cassert>
#include <utility>


#define MAX_VERSION 30000


using namespace argennon;
using namespace argennon::ascee::runtime;
using std::vector, std::move;


constexpr uint32 max_chunk_size = 64 * 1024;


int16_t RestrictedModifier::saveVersion() {
    if (currentVersion == MAX_VERSION) throw AsceeError("version limit reached", StatusCode::limit_exceeded);
    return currentVersion++;
}

void RestrictedModifier::restoreVersion(int16_t version) {
    assert(version < currentVersion && version >= 0);
    currentVersion = version;
}

void RestrictedModifier::loadChunk(long_id localID) {
    loadChunk(0, localID);
}

void RestrictedModifier::loadChunk(long_id accountID, long_id localID) {
    try {
        currentChunk = &chunks->at({accountID, localID});
    } catch (const std::out_of_range&) {
        throw std::out_of_range("chunk[" + (std::string) accountID + "." + (std::string) localID + "] is not defined");
    }
}

void RestrictedModifier::loadContext(long_id appID) {
    // When `appID` does not exist in the map, this function should not throw an exception. Smart contracts do not
    // call this function directly and failing can be problematic for `invoke_dispatcher` function.
    try {
        chunks = &appsAccessMaps.at(appID);
    } catch (const std::out_of_range&) {
        chunks = nullptr;
    }
    currentChunk = nullptr;
}

void RestrictedModifier::writeToHeap() {
    if (currentVersion == 0) return;

    for (auto& appMap: appsAccessMaps.getValues()) {
        for (auto& chunk: appMap.getValues()) {
            auto chunkSize = chunk.size.read<uint32>(currentVersion, 0);
            if (chunk.resizing == ChunkInfo::ResizingType::expandable ||
                chunk.resizing == ChunkInfo::ResizingType::shrinkable) {
                chunk.ptr->setSize(int(chunkSize));
            }
            if (chunkSize > 0 && chunk.ptr->isWritable()) {
                for (long i = 0; i < chunk.accessTable.size(); ++i) {
                    auto offset = chunk.accessTable.getKeys()[i];
                    if (offset >= chunkSize) break;     // since offsets are sorted.
                    // We should make sure that we never write outside the chunk.
                    chunk.accessTable.getValues()[i].wrToHeap(chunk.ptr, currentVersion, chunkSize - offset);
                }
            }
        }
    }
}

void RestrictedModifier::updateChunkSize(uint32 newSize) {
    if (newSize == currentChunk->size.read<uint32>(currentVersion, 0)) return;

    if (currentChunk->resizing == ChunkInfo::ResizingType::expandable) {
        if (newSize < currentChunk->getInitialSize() || newSize > currentChunk->sizeBound) {
            throw std::out_of_range("invalid chunk size for expanding");
        }
    } else if (currentChunk->resizing == ChunkInfo::ResizingType::shrinkable) {
        if (newSize > currentChunk->getInitialSize() || newSize < currentChunk->sizeBound) {
            throw std::out_of_range("invalid chunk size for shrinking");
        }
    } else {
        throw std::out_of_range("chunk is not resizable");
    }
    currentChunk->size.write(currentVersion, 0, newSize);
}

uint32 RestrictedModifier::getChunkSize() {
    if (currentChunk->resizing == ChunkInfo::ResizingType::non_accessible) {
        throw std::out_of_range("chunkSize is not accessible");
    }
    return currentChunk->size.read<uint32>(currentVersion, 0);
}

void RestrictedModifier::AccessBlock::syncTo(int16_t version) {
    while (!versionList.empty() && versionList.back().number > version) {
        versionList.pop_back();
    }
}

bool RestrictedModifier::AccessBlock::ensureExists(int16_t version) {
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

byte* RestrictedModifier::AccessBlock::prepareToRead(int16_t version, uint32 offset, uint32 readSize) {
    if (offset + readSize > size) throw std::out_of_range("out of block read");
    if (accessType.denies(BlockAccessInfo::Access::Operation::read)) {
        throw std::out_of_range("access block is not readable");
    }
    syncTo(version);
    return versionList.empty() ? heapLocation.get(readSize) + offset : versionList.back().getContent() + offset;
}

byte* RestrictedModifier::AccessBlock::prepareToWrite(int16_t version, uint32 offset, uint32 writeSize) {
    // todo IMPORTANT OVERFLOW PROBLEM! we have this issue in many places
    if (offset + writeSize > size) throw std::out_of_range("out of block write");
    if (accessType.denies(BlockAccessInfo::Access::Operation::write)) {
        throw std::out_of_range("block is not writable");
    }
    syncTo(version);
    auto oldContent = versionList.empty() ? heapLocation.get(size) : versionList.back().getContent();
    bool added = ensureExists(version);
    if (added && size != writeSize) {
        memcpy(versionList.back().getContent(), oldContent, offset);
        memcpy(versionList.back().getContent() + offset + writeSize, oldContent + offset + writeSize,
               size - writeSize - offset);
    }

    return versionList.back().getContent() + offset;
}

void RestrictedModifier::AccessBlock::wrToHeap(Chunk* chunk, int16_t version, uint32 maxWriteSize) {
    syncTo(version);
    if (versionList.empty()) return;

    auto writeSize = std::min(size, maxWriteSize);

    if (accessType.isAdditive()) {
        int64_fast s = 0, a = 0;
        assert(size <= sizeof(int64_t));
        std::lock_guard<std::mutex> lock(chunk->getContentMutex());
        memcpy(&s, heapLocation.get(size), size);
        memcpy(&a, versionList.back().getContent(), size);
        s += a;
        // mem copy should be inside this if block to make sure that lock_guard is protecting it.
        memcpy(heapLocation.get(writeSize), &s, writeSize);
    } else {
        memcpy(heapLocation.get(writeSize), versionList.back().getContent(), writeSize);
    }
}

RestrictedModifier::AccessBlock::AccessBlock(const Chunk::Pointer& heapLocation,
                                             int32 size,
                                             BlockAccessInfo::Access accessType) : heapLocation(heapLocation),
                                                                                   size(size),
                                                                                   accessType(accessType) {}

RestrictedModifier::ChunkInfo::ChunkInfo(Chunk* chunk, ResizingType resizingType, uint32 sizeBound,
                                         const std::vector<int32>& sortedAccessedOffsets,
                                         const std::vector<BlockAccessInfo>& accessInfoList) :
// We do not check inputs. We assume that all inputs are checked by upper layers.
        size(
                Chunk::Pointer((byte*) (&initialSize), sizeof(initialSize)),
                sizeof(initialSize),
                BlockAccessInfo::Access(
                        resizingType == ResizingType::expandable || resizingType == ResizingType::shrinkable ?
                        BlockAccessInfo::Access::Type::writable : BlockAccessInfo::Access::Type::read_only)
        ),
        sizeBound(sizeBound),
        ptr(chunk),
        resizing(resizingType),
        accessTable(toAccessBlocks(chunk, sortedAccessedOffsets, accessInfoList)) {
    assert(sizeBound <= max_chunk_size);
    initialSize = chunk->getsize();
}

RestrictedModifier::AccessTableMap RestrictedModifier::ChunkInfo::toAccessBlocks(
        Chunk* chunk,
        const vector<int32>& offsets,
        const vector<BlockAccessInfo>& accessInfoList
) {
    vector<AccessBlock> blocks;
    blocks.reserve(offsets.size());
    vector<uint32> resultOffsets;
    resultOffsets.reserve(offsets.size());

    for (int i = 0; i < offsets.size(); ++i) {
        if (accessInfoList[i].accessType.mayWrite() && !chunk->isWritable()) {
            throw BlockError("trying to modify a readonly chunk");
        }

        if (offsets[i] >= 0) {
            resultOffsets.emplace_back(offsets[i]);
            blocks.emplace_back(
                    chunk->getContentPointer(offsets[i], accessInfoList[i].size),
                    accessInfoList[i].size,
                    accessInfoList[i].accessType
            );
        }
    }
    return {std::move(resultOffsets), std::move(blocks)};
}
