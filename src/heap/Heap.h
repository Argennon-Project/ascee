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
#include "Chunk.h"
#include "Page.h"

namespace ascee {

struct AppMemAccess {
    struct Block {
        int32 offset;
        int32 size;
        bool writable;
    };

    struct Chunk {
        std_id_t id;
        int32 maxNewSize;
        std::vector<Block> accessBlocks;
    };

    std_id_t appID;
    std::vector<Chunk> chunks;
};

//TODO: Heap must be signal-safe but it does not need to be thread-safe
class Heap {
private:
    std::unordered_map<int128, std::unique_ptr<Chunk>> tempArea;
    std::unordered_map<int128, Chunk*> chunkIndex;
    std::unordered_map<int128, Page> pageCache;

    Chunk* getChunk(std_id_t appID, std_id_t chunkID);

    Chunk* newChunk(std_id_t appID, std_id_t id, int32 size);

public:
    class Modifier {
        friend class Heap;

    private:
        class AccessBlock {
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

        private:
            bool writable;
            std::vector<Version> versionList;

            void syncTo(int16_t version);

            bool add(int16_t version);

        public:
            AccessBlock(Chunk::Pointer heapLocation, int32 size, bool writable);

            AccessBlock(const AccessBlock&) = delete;

            [[nodiscard]] int32 getSize() const { return size; }

            template<typename T>
            inline
            T read(int16_t version) {
                if (sizeof(T) > size) throw std::out_of_range("read size");
                syncTo(version);
                if (versionList.empty()) return heapLocation.read<T>();
                return *(T*) versionList.back().content;
            }

            template<typename T>
            inline
            void write(int16_t version, T value) {
                if (sizeof(T) != size) throw std::out_of_range("write size");
                if (!writable) throw std::out_of_range("block is not writable");
                syncTo(version);
                add(version);
                *(T*) versionList.back().content = value;
            }

            void wrToHeap(int16_t version);
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

        typedef std::unordered_map<std_id_t, ChunkInfo> ChunkMap64;
        typedef std::unordered_map<std_id_t, ChunkMap64> AppMap;

        AccessTableMap* accessTable = nullptr;
        ChunkInfo* currentChunk = nullptr;
        ChunkMap64* chunks = nullptr;
        AppMap appsAccessMaps;

        explicit Modifier(Heap* parent) : parent(parent) {}

        void defineChunk(std_id_t ownerApp, std_id_t chunkID, Chunk::Pointer sizePtr, int32 maxSize = -1);

        void defineAccessBlock(Chunk::Pointer heapLocation,
                               std_id_t app, std_id_t chunk, int32 offset,
                               int32 size, bool writable);

    public:
        template<typename T>
        inline
        T load(int32 offset) { return accessTable->at(offset).read<T>(currentVersion); }

        template<typename T>
        inline
        void store(int32 offset, T value) { accessTable->at(offset).write<T>(currentVersion, value); }

        void loadChunk(short_id_t chunkID);

        void loadChunk(std_id_t chunkID);

        void loadContext(std_id_t appID);

        void restoreVersion(int16_t version);

        int16_t saveVersion();

        void writeToHeap();

        int32 getChunkSize();

        void updateChunkSize(int32 newSize);
    };

    Heap();

    Modifier* initSession(std_id_t calledApp);

    Modifier* initSession(const std::vector<AppMemAccess>& memAccessList);

    void freeChunk(std_id_t appID, std_id_t id);
};

} // namespace ascee

#endif // ASCEE_HEAP_H
