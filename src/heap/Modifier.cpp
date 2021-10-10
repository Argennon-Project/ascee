
#include "Heap.h"
// #define NDEBUG
#include <cassert>

#define MAX_VERSION 30000

using namespace ascee;

/*
uint8_t HeapModifier::loadByte(int32_t offset) {
    return accessTable.at(offset & ~0b111).heapLocation[offset & 0b111];
}*/

int16_t Heap::Modifier::saveVersion() {
    if (currentVersion == MAX_VERSION) throw std::out_of_range("version limit reached");
    return currentVersion++;
}

void Heap::Modifier::restoreVersion(int16_t version) {
    if (version > currentVersion || version < 0) throw std::runtime_error("restoring an invalid version");
    currentVersion = version;
}

void Heap::Modifier::loadChunk(short_id_t chunkID) {
    accessTable = &chunks32->at(chunkID);
}

void Heap::Modifier::loadChunk(std_id_t chunkID) {
    accessTable = &chunks64->at(chunkID);
}

void Heap::Modifier::loadContext(std_id_t appID) {
    // When `appID` does not exist in the map, this function should not throw an exception. Smart contracts do not
    // call this function directly and failing can be problematic for `invoke_dispatcher` function.
    chunks32 = &appsAccessMaps[appID].first;
    chunks64 = &appsAccessMaps[appID].second;
    accessTable = nullptr;
}

void Heap::Modifier::defineAccessBlock(Pointer heapLocation,
                                       std_id_t app, short_id_t chunk, int32 offset,
                                       int32 size, bool writable) {
    appsAccessMaps[app].first[chunk].emplace(std::piecewise_construct,
                                             std::forward_as_tuple(offset),
                                             std::forward_as_tuple(heapLocation, size, writable));
}

void Heap::Modifier::defineAccessBlock(Pointer heapLocation,
                                       std_id_t app, std_id_t chunk, int32 offset,
                                       int32 size, bool writable) {
    appsAccessMaps[app].second[chunk].emplace(std::piecewise_construct,
                                              std::forward_as_tuple(offset),
                                              std::forward_as_tuple(heapLocation, size, writable));
}

void Heap::Modifier::AccessBlock::syncTo(int16_t version) {
    while (!versionList.empty() && versionList.back().number > version) {
        versionList.pop_back();
    }
}

bool Heap::Modifier::AccessBlock::add(int16_t version) {
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

Heap::Modifier::AccessBlock::AccessBlock(Pointer heapLocation, int32 size, bool writable) : heapLocation(heapLocation),
                                                                                            size(size),
                                                                                            writable(writable) {}
