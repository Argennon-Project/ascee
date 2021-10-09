
#ifndef ASCEE_HEAP_MODIFIER_H
#define ASCEE_HEAP_MODIFIER_H

#include "argc/types.h"
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

namespace ascee {

class HeapModifier {
    friend class Heap;

private:
    int16_t currentVersion = 0;
    uint8_t tempHeap[64] = {0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

    struct AccessBlock {
        // private:
        uint8_t* heapLocation;
        int32_t size;

        struct Snapshot {
            int16_t version;
            uint8_t* content;

            Snapshot(int16_t version, int32_t size) : version(version) { content = new uint8_t[size]; }

            ~Snapshot() { delete content; }
        };

        std::vector<Snapshot*> snapshotList;

    public:
        template<typename T>
        inline
        T read() {
            if (sizeof(T) > size) throw std::out_of_range("read size");
            return *(T*) heapLocation;
        }

        template<typename T>
        inline
        void write(T value) {
            if (sizeof(T) > size) throw std::out_of_range("write size");
            auto* heapPtr = (T*) heapLocation;
            *heapPtr = value;
        }

        void syncTo(int16_t version);

        void updateTo(int16_t version);

        void smartCopy(uint8_t* dst, uint8_t* src);
    };

    typedef std::unordered_map<int32_t, AccessBlock> AccessTableMap;
    typedef std::unordered_map<argc::short_id_t, AccessTableMap> ChunkMap32;
    typedef std::unordered_map<argc::std_id_t, AccessTableMap> ChunkMap64;
    typedef std::unordered_map<argc::std_id_t, std::pair<ChunkMap32, ChunkMap64>> AppMap;


    AccessTableMap dummyTable;
    ChunkMap32 dummy32;
    ChunkMap64 dummy64;

    AccessTableMap& accessTable = dummyTable;
    ChunkMap64& chunks64 = dummy64;
    ChunkMap32& chunks32 = dummy32;
    AppMap appsData;


public:

    HeapModifier(argc::std_id_t appID) {
        appsData[appID] = std::pair(ChunkMap32(), ChunkMap64());
        appsData[appID].first[111] = {{5, {tempHeap + 5, 8}}};
    }

    template<typename T>
    inline
    T load(int32_t offset) {
        auto block = accessTable.at(offset);
        block.syncTo(currentVersion);
        return block.read<T>();
    }

    template<typename T>
    inline
    void store(int32_t offset, T value) {
        auto block = accessTable.at(offset);
        block.syncTo(currentVersion);
        block.updateTo(currentVersion);
        block.write<T>(value);
    }

    inline
    void loadChunk(argc::short_id_t chunkID) { accessTable = chunks32.at(chunkID); }

    inline
    void loadChunk(argc::std_id_t chunkID) { accessTable = chunks64.at(chunkID); }

    inline
    void changeContext(argc::std_id_t appID) {
        chunks32 = appsData.at(appID).first;
        chunks64 = appsData.at(appID).second;
    }

    void restoreVersion(int16_t version);

    int16_t saveVersion();

    // void closeContextNormally(argc::std_id_t from, argc::std_id_t to);

    // void closeContextAbruptly(argc::std_id_t from, argc::std_id_t to);

    ~HeapModifier() {
        std::cout << "mod:dest" << std::endl;
    }
};

} // namespace ascee
#endif // ASCEE_HEAP_MODIFIER_H

