
#ifndef ASCEE_HEAP_H
#define ASCEE_HEAP_H

#include <argc/types.h>
#include <unordered_map>
#include <vector>

namespace ascee {

class Heap {
private:
    byte content[64] = {0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

    class Pointer {
        friend class Heap;

    private:
        byte* heapPtr;

        explicit Pointer(byte* heapPtr) : heapPtr(heapPtr) {}

    public:
        template<typename T>
        inline
        T read() { return *(T*) heapPtr; }

        template<typename T>
        inline
        void write(T value) { *(T*) heapPtr = value; }

        void readBlock(byte* dst, int32 size);

        void writeBlock(const byte* src, int32 size);
    };

public:
    class Modifier {
        friend class Heap;

    private:
        int16_t currentVersion = 0;

        class AccessBlock {
        private:
            Heap::Pointer heapLocation;
            int32 size;

            struct Snapshot {
                int16_t version;
                byte* content;

                Snapshot(int16_t version, int32 size) : version(version) { content = new byte[size]; }

                ~Snapshot() { delete content; }
            };

            std::vector<Snapshot*> snapshotList;

        public:
            AccessBlock(Pointer heapLocation, int32 size, bool writable);

            template<typename T>
            inline
            T read() {
                if (sizeof(T) > size) throw std::out_of_range("read size");
                return heapLocation.read<T>();
            }

            template<typename T>
            inline
            void write(T value) {
                if (sizeof(T) > size) throw std::out_of_range("write size");
                heapLocation.write(value);
            }

            void syncTo(int16_t version);

            void updateTo(int16_t version);
        };

        typedef std::unordered_map<int32, AccessBlock> AccessTableMap;
        typedef std::unordered_map<short_id_t, AccessTableMap> ChunkMap32;
        typedef std::unordered_map<std_id_t, AccessTableMap> ChunkMap64;
        typedef std::unordered_map<std_id_t, std::pair<ChunkMap32, ChunkMap64>> AppMap;

        AccessTableMap dummyTable;
        ChunkMap32 dummy32;
        ChunkMap64 dummy64;
        AccessTableMap& accessTable = dummyTable;
        ChunkMap64& chunks64 = dummy64;
        ChunkMap32& chunks32 = dummy32;
        AppMap appsAccessMaps;

        Modifier() = default;

        void defineAccessBlock(Pointer heapLocation,
                               std_id_t app, short_id_t chunk, int32 offset,
                               int32 size, bool writable);

    public:
        template<typename T>
        inline
        T load(int32 offset) {
            auto block = accessTable.at(offset);
            block.syncTo(currentVersion);
            return block.read<T>();
        }

        template<typename T>
        inline
        void store(int32 offset, T value) {
            auto block = accessTable.at(offset);
            block.syncTo(currentVersion);
            block.updateTo(currentVersion);
            block.write<T>(value);
        }

        void loadChunk(short_id_t chunkID);

        void loadChunk(std_id_t chunkID);

        void loadContext(std_id_t appID);

        void restoreVersion(int16_t version);

        int16_t saveVersion();
    };

    Modifier* initSession(std_id_t calledApp);
};

} // namespace ascee

#endif // ASCEE_HEAP_H
