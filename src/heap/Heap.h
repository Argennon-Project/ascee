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
    byte zero32[4] = {0};
    byte page[128 * 1024] = {0};
    byte* freeArea = page;
    std::unordered_map<std_id_t, byte*> chunkIndex;

    class Pointer {
        friend class Heap;

    private:
        byte* heapPtr;

        explicit Pointer(byte* heapPtr) : heapPtr(heapPtr) {}

    public:
        template<typename T>
        inline
        T read() { return *(T*) heapPtr; }

        inline bool isNull() { return heapPtr == nullptr; }

        void readBlockTo(byte* dst, int32 size);

        void writeBlock(const byte* src, int32 size);
    };

    Pointer getSizePointer(std_id_t appID, std_id_t chunkID);

    Pointer newChunk(std_id_t appID, std_id_t id, int32 size);

    Pointer newTransientChunk();

    void saveChunk(Pointer sizePtr);

    void freeChunk(Pointer sizePtr);

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
                    printf("deleteeeed\n");
                    delete[] content;
                }
            };


            Heap::Pointer heapLocation;
            int32 size;

        private:
            bool writable;
            std::vector<Version> versionList;

            void syncTo(int16_t version);

            bool add(int16_t version);

        public:
            AccessBlock(Pointer heapLocation, int32 size, bool writable);

            AccessBlock(const AccessBlock&) = delete;

            [[nodiscard]] int32 getSize() const { return size; }

            Pointer getHeapPointer() { return heapLocation; }

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
        typedef std::unordered_map<short_id_t, AccessTableMap> ChunkMap32;
        typedef std::unordered_map<std_id_t, AccessTableMap> ChunkMap64;
        typedef std::unordered_map<std_id_t, std::pair<ChunkMap32, ChunkMap64>> AppMap;

        AccessTableMap* accessTable = nullptr;
        ChunkMap64* chunks64 = nullptr;
        ChunkMap32* chunks32 = nullptr;
        AppMap appsAccessMaps;

        Modifier(Heap* parent) : parent(parent) {}

        void defineAccessBlock(Pointer heapLocation,
                               std_id_t app, short_id_t chunk, int32 offset,
                               int32 size, bool writable);

        void defineAccessBlock(Pointer heapLocation,
                               std_id_t app, std_id_t chunk, int32 offset,
                               int32 size, bool writable);

        void writeTable(AccessTableMap& table);

    public:
        static const int sizeCell = -4;
        static const int maxNewSizeCell = -5;

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
};

} // namespace ascee

#endif // ASCEE_HEAP_H
