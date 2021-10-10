
#ifndef ASCEE_HEAP_H
#define ASCEE_HEAP_H

#include <argc/types.h>
#include <unordered_map>
#include <vector>

namespace ascee {
//todo Heap must be signal-safe but it does not need to be thread-safe
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

        void readBlockTo(byte* dst, int32 size);

        void writeBlock(const byte* src, int32 size);
    };

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
            bool writable;
            std::vector<Version> versionList;

            void syncTo(int16_t version);

            bool add(int16_t version);

        public:
            AccessBlock(Pointer heapLocation, int32 size, bool writable);

            AccessBlock(const AccessBlock&) = delete;

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
                if (sizeof(T) > size) throw std::out_of_range("write size");
                if (!writable) throw std::out_of_range("block is not writable");
                syncTo(version);
                bool versionCreated = add(version);
                if (versionCreated && sizeof(T) != size) {
                    heapLocation.readBlockTo(versionList.back().content, size);
                }
                *(T*) versionList.back().content = value;
            }
        };

        int16_t currentVersion = 0;

        typedef std::unordered_map<int32, AccessBlock> AccessTableMap;
        typedef std::unordered_map<short_id_t, AccessTableMap> ChunkMap32;
        typedef std::unordered_map<std_id_t, AccessTableMap> ChunkMap64;
        typedef std::unordered_map<std_id_t, std::pair<ChunkMap32, ChunkMap64>> AppMap;

        AccessTableMap* accessTable = nullptr;
        ChunkMap64* chunks64 = nullptr;
        ChunkMap32* chunks32 = nullptr;
        AppMap appsAccessMaps;

        Modifier() = default;

        void defineAccessBlock(Pointer heapLocation,
                               std_id_t app, short_id_t chunk, int32 offset,
                               int32 size, bool writable);

        void defineAccessBlock(Pointer heapLocation,
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
    };

    Modifier* initSession(std_id_t calledApp);
};

} // namespace ascee

#endif // ASCEE_HEAP_H
