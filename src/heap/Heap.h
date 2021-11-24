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

#ifndef ASCEE_HEAP_H
#define ASCEE_HEAP_H


#include <argc/types.h>
#include <unordered_map>
#include <vector>
#include <util/IdentifierTrie.h>
#include "Chunk.h"
#include "Page.h"

namespace ascee::runtime {

struct AppMemAccess {
    struct Block {
        int32 offset;
        int32 size;
        bool writable;
    };

    struct Chunk {
        long_id id;
        int32 maxNewSize;
        std::vector<Block> accessBlocks;
    };

    long_id appID;
    std::vector<Chunk> chunks;
};

//TODO: Heap must be signal-safe but it does not need to be thread-safe
class Heap {
private:
    std::unordered_map<int128, Chunk*> chunkIndex;
    std::unordered_map<int128, Page> pageCache;

    Chunk* getChunk(long_id appID, long_id chunkID);

    Chunk* newChunk(long_id appID, long_id id, int32 size);

public:
    class Modifier {
        friend class Heap;

    private:
        class AccessBlock {

        public:
            AccessBlock(Chunk::Pointer heapLocation, int32 size, bool writable);

            AccessBlock(const AccessBlock&) = delete;

            [[nodiscard]] int32 getSize() const { return size; }

            template<typename T, int h>
            inline
            T readIdentifier(const IdentifierTrie<T, h>& trie, int16_t version, int* n = nullptr) {
                auto content = syncTo(version);
                return trie.readIdentifier(content, n, size);
            }

            template<typename T, int h>
            inline
            T readVarUInt(const IdentifierTrie<T, h>& trie, int16_t version, int* n = nullptr) {
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
            int writeVarUInt(const IdentifierTrie<T, h>& trie, int16_t version, T value) {
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

            bool add(int16_t version);

            void prepareToWrite(int16_t version, int writeSize);
        };

        Heap* parent;

    private:
        int16_t currentVersion = 0;

        typedef std::unordered_map<int32, AccessBlock> AccessTableMap;

        struct ChunkInfo {
            AccessTableMap accessTable;
            AccessBlock size;
            const int32 maxSize;

            explicit ChunkInfo(Chunk::Pointer sizePtr, const int32 maxSize = -1) :
                    size(sizePtr, sizeof(int32), maxSize >= 0),
                    maxSize(maxSize) {}
        };

        typedef std::unordered_map<long_id, ChunkInfo> ChunkMap64;
        typedef std::unordered_map<long_id, ChunkMap64> AppMap;

        AccessTableMap* accessTable = nullptr;
        ChunkInfo* currentChunk = nullptr;
        ChunkMap64* chunks = nullptr;
        AppMap appsAccessMaps;

        explicit Modifier(Heap* parent) : parent(parent) {}

        void defineChunk(long_id ownerApp, long_id chunkID, Chunk::Pointer sizePtr, int32 maxSize = -1);

        void defineAccessBlock(Chunk::Pointer heapLocation,
                               long_id app, long_id chunk, int32 offset,
                               int32 size, bool writable);

    public:
        template<typename T>
        inline
        T load(int32 offset) { return accessTable->at(offset).read<T>(currentVersion); }

        template<typename T, int h>
        inline
        T loadVarUInt(const IdentifierTrie<T, h>& trie, int32 offset, int* n = nullptr) {
            return accessTable->at(offset).readVarUInt(trie, currentVersion, n);
        }

        template<typename T, int h>
        inline
        T loadIdentifier(const IdentifierTrie<T, h>& trie, int32 offset, int* n = nullptr) {
            return accessTable->at(offset).readIdentifier(trie, currentVersion, n);
        }

        template<typename T>
        inline
        void store(int32 offset, T value) { accessTable->at(offset).write<T>(currentVersion, value); }

        template<typename T, int h>
        inline
        int storeVarUInt(const IdentifierTrie<T, h>& trie, int32 offset, T value) {
            return accessTable->at(offset).writeVarUInt(trie, currentVersion, value);
        }

        void loadChunk(short_id chunkID);

        void loadChunk(long_id chunkID);

        void loadContext(long_id appID);

        void restoreVersion(int16_t version);

        int16_t saveVersion();

        void writeToHeap();

        int32 getChunkSize();

        void updateChunkSize(int32 newSize);
    };

    Heap();

    Modifier* initSession(long_id calledApp);

    Modifier* initSession(const std::vector<AppMemAccess>& memAccessList);

    void freeChunk(long_id appID, long_id id);
};

} // namespace ascee::runtime

#endif // ASCEE_HEAP_H
