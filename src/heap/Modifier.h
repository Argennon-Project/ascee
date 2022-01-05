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

    bool isValid(int32 offset, int32 size) {
        // we need to make sure that the access block is defined. Otherwise, scheduler can not guarantee that
        // isValid is properly parallelized with chunk resizing requests.
        if (size > currentChunk->accessTable.at(offset).getSize()) {
            throw std::out_of_range("isValid size");
        }
        // we can't use getChunkSize() here
        return offset < currentChunk->size.read<int32>(currentVersion);
    }

    int32 getChunkSize();

    void updateChunkSize(int32 newSize);

private:
    class AccessBlock {
    public:
        AccessBlock() = default;

        AccessBlock(AccessBlock&&) = default;

        AccessBlock(const AccessBlock&) = delete;

        AccessBlock(const Chunk::Pointer& heapLocation, int32 size, bool writable);

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

        void wrToHeap(int16_t version, int32 maxWriteSize);


    private:
        struct Version {
            const int16_t number;
            std::unique_ptr<byte[]> content;

            inline byte* getContent() { return content.get(); } // NOLINT(readability-make-member-function-const)

            Version(int16_t version, int32 size) : number(version), content(std::make_unique<byte[]>(size)) {}
        };

        Chunk::Pointer heapLocation;
        int32 size = 0;
        bool writable = false;
        std::vector<Version> versionList;

        byte* syncTo(int16_t version);

        byte* add(int16_t version);

        void prepareToWrite(int16_t version, int writeSize);
    };

    typedef util::FixedOrderedMap <int32, AccessBlock> AccessTableMap;
public:
    class ChunkInfo {
        friend class Modifier;

    private:
        AccessTableMap accessTable;
        AccessBlock size;
        /// When the size AccessBlock is writable and newSize > 0 the chunk can only be expanded and the value of
        /// newSize indicates the upper bound of the settable size. If newSize <= 0 the chunk is only shrinkable and
        /// the magnitude of newSize indicates the lower bound of the chunk's new size.
        /// When the size AccessBlock is not writable, newSize != 0 indicates that the size block is readable and
        /// newSize == 0 indicates that the size block is not accessible.
        const int32 newSize = 0;
        Chunk* ptr{};
        int32 initialSize;

        static std::vector<AccessBlock> toBlocks(
                Chunk* chunk,
                const std::vector<int32>& offsets,
                const std::vector<BlockAccessInfo>& accessInfoList
        );

    public:
        ChunkInfo(
                Chunk* chunk, int32 newSize, bool resizable,
                std::vector<int32> sortedAccessedOffsets,
                const std::vector<BlockAccessInfo>& accessInfoList
        );

        ChunkInfo(ChunkInfo&&) = default;

        ChunkInfo(const ChunkInfo&) = delete;

        [[nodiscard]] int32 getInitialSize() const {
            // initial sizeChunk will not be modified as long as wrToHeap function is not called, all changes are only
            // made to the version array. Therefor we can obtain the initial size this way.
            return initialSize;
        }
    };

    typedef util::FixedOrderedMap <long_id, ChunkInfo> ChunkMap64;

    Modifier(std::vector<long_id> apps, std::vector<ChunkMap64> chunkMaps) :
            appsAccessMaps(std::move(apps), std::move(chunkMaps)) {}

    Modifier() = default;

    Modifier(const Modifier&) = delete;

    Modifier(Modifier&&) = delete;

private:
    typedef util::FixedOrderedMap <long_id, ChunkMap64> AppMap;

    int16_t currentVersion = 0;
    ChunkInfo* currentChunk = nullptr;
    ChunkMap64* chunks = nullptr;
    AppMap appsAccessMaps;
};


} // namespace ascee::runtime::heap
#endif // ASCEE_HEAP_MODIFIER_H
