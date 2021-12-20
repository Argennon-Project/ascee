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

#ifndef ASCEE_HEAP_MODIFIER_H
#define ASCEE_HEAP_MODIFIER_H

#include <exception>
#include <cstring>
#include <mutex>
#include <vector>
#include "argc/primitives.h"
#include "Chunk.h"
#include "util/IdentifierTrie.h"

namespace ascee::runtime::heap {

class Modifier {
public:
    Modifier() = default;

    Modifier(const Modifier&) { std::terminate(); }

    Modifier(Modifier&&) = delete;

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

    void
    defineChunk(long_id ownerApp, long_id chunkID, Chunk* chunkOnHeap, int32 newSize, bool resizable);

    void defineAccessBlock(long_id app, long_id chunk, int32 offset,
                           int32 size, bool writable);


private:
    class AccessBlock {

    public:
        AccessBlock(Chunk::Pointer heapLocation, int32 size, bool writable);

        AccessBlock(const AccessBlock&) = delete;

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
            trie.writeBigEndian(versionList.back().content, code, len);
            return len;
        }


        template<typename T>
        inline
        void write(int16_t version, T value) {
            prepareToWrite(version, sizeof(value));
            memcpy(versionList.back().content, (byte*) &value, sizeof(value));
        }

        void wrToHeap(int16_t version);


    private:
        struct Version {
            const int16_t number;
            byte* content;

            Version(int16_t version, int32 size) : number(version), content(new byte[size]) {}

            Version(const Version&) = delete;

            Version(Version&& moved) noexcept: number(moved.number), content(moved.content) {
                moved.content = nullptr;
            }

            ~Version() {
                printf("deleted\n");
                delete[] content;
            }
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

    typedef std::unordered_map<int32, AccessBlock> AccessTableMap;

    struct ChunkInfo {
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


        explicit ChunkInfo(Chunk* chunk, int32 newSize, bool resizable) :
                size(Chunk::Pointer((byte * )(&initialSize), (byte * ) & initialSize + sizeof(initialSize)),
                     sizeof(int32),
                     resizable),
                newSize(newSize), ptr(chunk) {
            initialSize = chunk->getsize();
        }
    };

    typedef std::unordered_map<long_id, ChunkInfo> ChunkMap64;
    typedef std::unordered_map<long_id, ChunkMap64> AppMap;

    ChunkInfo* currentChunk = nullptr;
    ChunkMap64* chunks = nullptr;
    AppMap appsAccessMaps;
};

} // namespace ascee::runtime::heap
#endif // ASCEE_HEAP_MODIFIER_H
