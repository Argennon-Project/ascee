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

#ifndef ASCEE_HEAP_MODIFIER_H
#define ASCEE_HEAP_MODIFIER_H

#include <exception>
#include <cstring>
#include <mutex>
#include <utility>
#include <vector>
#include "argc/primitives.h"
#include "Chunk.h"
#include "util/IdentifierTrie.h"
#include "util/FixedOrderedMap.hpp"
#include "loader/BlockLoader.h"

namespace ascee::runtime::heap {

class Modifier {
public:
    template<typename T>
    inline
    T load(int32 offset) { return currentChunk->accessTable.at(offset).read<T>(currentVersion); }

    template<typename T, int h>
    inline
    T loadVarUInt(const IdentifierTrie <T, h>& trie, int32 offset, int* n = nullptr) {
        return currentChunk->accessTable.at(offset).readVarUInt(trie, currentVersion, n);
    }

    template<typename T, int h>
    inline
    T loadIdentifier(const IdentifierTrie <T, h>& trie, int32 offset, int* n = nullptr) {
        return currentChunk->accessTable.at(offset).readIdentifier(trie, currentVersion, n);
    }

    template<typename T>
    inline
    void store(int32 offset, T value) { currentChunk->accessTable.at(offset).write<T>(currentVersion, value); }

    template<typename T, int h>
    inline
    int storeVarUInt(const IdentifierTrie <T, h>& trie, int32 offset, T value) {
        return currentChunk->accessTable.at(offset).writeVarUInt(trie, currentVersion, value);
    }

    void loadChunk(short_id chunkID);

    void loadChunk(long_id chunkID);

    void loadContext(long_id appID);

    void restoreVersion(int16_t version);

    int16_t saveVersion();

    void writeToHeap();

    int32 getChunkSize();

    void updateChunkSize(int32 newSize);

private:
    class AccessBlock {
    public:
        AccessBlock(Chunk::Pointer heapLocation, int32 size, bool writable);

        //todo AccessBlock(const AccessBlock&) = delete;

        [[nodiscard]] bool isWritable() const {
            return writable;
        }

        [[nodiscard]] inline
        int32 getSize() const { return size; }

        template<typename T, int h>
        inline
        T readIdentifier(const IdentifierTrie <T, h>& trie, int16_t version, int* n = nullptr) {
            auto content = syncTo(version);
            return trie.readIdentifier(content, n, size);
        }

        template<typename T, int h>
        inline
        T readVarUInt(const IdentifierTrie <T, h>& trie, int16_t version, int* n = nullptr) {
            auto content = syncTo(version);
            return trie.decodeVarUInt(content, n, size);
        }

        template<typename T>
        inline
        T read(int16_t version) {
            if (sizeof(T) > size) throw std::out_of_range("read size");
            auto content = syncTo(version);
            T ret{};
            memcpy((byte*) &ret, content, sizeof(T));
            // Here we return the result by value. This is not bad for performance. Compilers usually implement
            // return-by-value using pass-by-pointer. (a.k.a NRVO optimization)
            return ret;
        }

        template<typename T, int h>
        inline
        int writeVarUInt(const IdentifierTrie <T, h>& trie, int16_t version, T value) {
            int len;
            auto code = trie.encodeVarUInt(value, &len);
            prepareToWrite(version, len);
            trie.writeBigEndian(versionList.back().getContent(), code, len);
            return len;
        }


        template<typename T>
        inline
        void write(int16_t version, T value) {
            prepareToWrite(version, sizeof(value));
            memcpy(versionList.back().getContent(), (byte*) &value, sizeof(value));
        }

        void wrToHeap(int16_t version);


    private:
        struct Version {
            const int16_t number;
            std::unique_ptr<byte[]> content;

            inline
            byte* getContent() { return content.get(); }

            Version(int16_t version, int32 size) : number(version), content(std::make_unique<byte[]>(size)) {}
        };

        Chunk::Pointer heapLocation;
        int32 size;
        bool writable;
        std::vector<Version> versionList;

        byte* syncTo(int16_t version);

        byte* add(int16_t version);

        void prepareToWrite(int16_t version, int writeSize);
    };

    int16_t currentVersion = 0;
    // this is not efficient and can be improved
    std::mutex writeMutex;

    typedef util::FixedOrderedMap <int32, AccessBlock> AccessTableMap;

public:
    class ChunkInfo {
        friend class Modifier;

    private:
        AccessTableMap accessTable;
        AccessBlock size;
        /// When the size AccessBlock is writable and newSize > 0 the chunk can only be expanded and the value of
        /// newSize indicates the upper bound of the settable size. If newSize <= 0 the chunk is only shrinkable and
        /// the magnitude of newSize indicates the lower bound of chunk's new size.
        /// When the size block is not writable. newSize >= 0 indicates that size block is readable but if newSize < 0
        /// the size block is not accessible.
        const int32 newSize;
        int32 initialSize;
        Chunk* ptr;

        static std::vector<AccessBlock> toBlocks(Chunk* chunk, const std::vector<BlockAccessInfo>& accessInfoList) {
            std::vector<AccessBlock> blocks;
            blocks.reserve(accessInfoList.size());
            for (const auto& info: accessInfoList) {
                //todo change to std::invalid_argument?
                if (info.writable && !chunk->isWritable()) throw BlockError("trying to modify a readonly chunk");
                blocks.emplace_back(chunk->getContentPointer(info.offset), info.size, info.writable);
            }
            return blocks;
        }

    public:
        /// When resizeable is true and newSize > 0 the chunk can only be expanded and the value of
        /// newSize indicates the upper bound of the settable size. If resizable == true and newSize <= 0 the chunk is only
        /// shrinkable and the magnitude of newSize indicates the lower bound of the chunk's new size.
        ///
        /// When resizeable is false and newSize >= 0 the size of the chunk can only be read but if newSize < 0
        /// the size of the chunk is not accessible. (not readable nor writable)
        explicit ChunkInfo(Chunk* chunk, int32 newSize, bool resizable,
                           std::vector<int32> sortedAccessedOffsets,
                           const std::vector<BlockAccessInfo>& accessInfoList) :
                size(
                        Chunk::Pointer((byte * )(&initialSize), (byte * ) & initialSize + sizeof(initialSize)),
                        sizeof(int32),
                        resizable
                ),
                newSize(newSize),
                ptr(chunk),
                accessTable(std::move(sortedAccessedOffsets), toBlocks(chunk, accessInfoList)) {
            initialSize = chunk->getsize();
        }
    };

    typedef util::FixedOrderedMap <long_id, ChunkInfo> ChunkMap64;

    Modifier() = default;

    Modifier(std::vector<long_id> apps, std::vector<ChunkMap64> chunkMaps) :
            appsAccessMaps(std::move(apps), std::move(chunkMaps)) {}

    Modifier(const Modifier&) { std::terminate(); }

    Modifier(Modifier&&) = delete;

private:
    typedef util::FixedOrderedMap <long_id, ChunkMap64> AppMap;

    ChunkInfo* currentChunk = nullptr;
    ChunkMap64* chunks = nullptr;
    AppMap appsAccessMaps;
};

} // namespace ascee::runtime::heap
#endif // ASCEE_HEAP_MODIFIER_H
