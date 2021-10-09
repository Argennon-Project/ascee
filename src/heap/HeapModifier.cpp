
#include "HeapModifier.h"
// #define NDEBUG
#include <cassert>

#define MAX_VERSION 30000

using namespace ascee;
using namespace argc;

template<typename T>
inline static
void copy(uint8_t* dst, uint8_t* src) {
    auto* dstPtr = (T*) dst;
    auto* srcPtr = (T*) src;
    *dstPtr = *srcPtr;
}

/*
uint8_t HeapModifier::loadByte(int32_t offset) {
    return accessTable.at(offset & ~0b111).heapLocation[offset & 0b111];
}*/

int16_t HeapModifier::saveVersion() {
    if (currentVersion == MAX_VERSION) throw std::out_of_range("version limit reached");
    return currentVersion++;
}

void HeapModifier::restoreVersion(int16_t version) {
    if (version > currentVersion || version < 0) throw std::runtime_error("restoring an invalid version");
    currentVersion = version;
}

void HeapModifier::AccessBlock::syncTo(int16_t version) {
    if (snapshotList.empty() || snapshotList.back()->version < version) return;

    while (snapshotList.back()->version > version) {
        delete snapshotList.back();
        snapshotList.pop_back();
    }

    // restore the snapshot data
    smartCopy(heapLocation, snapshotList.back()->content);

    if (snapshotList.back()->version == version) {
        delete snapshotList.back();
        snapshotList.pop_back();
    }
}

void HeapModifier::AccessBlock::updateTo(int16_t version) {
    // checks are ordered for having the best performance on average
    if (!snapshotList.empty()) {
        auto snapshotVersion = snapshotList.back()->version;
        assert(snapshotVersion <= version - 1);
        if (snapshotVersion == version - 1) return;
        if (snapshotVersion == -1) throw std::out_of_range("block is not writable");
    }

    if (version <= 0) return;

    snapshotList.push_back(new Snapshot(int16_t(version - 1), size));
    // copy heap into the newly created snapshot
    smartCopy(snapshotList.back()->content, heapLocation);
}

void HeapModifier::AccessBlock::smartCopy(uint8_t* dst, uint8_t* src) {
    switch (size) {
        case 8:
            copy<uint64_t>(dst, src);
            break;
        case 16:
            copy<int128>(dst, src);
            break;
        case 4:
            copy<uint32_t>(dst, src);
            break;
        default:
            throw std::runtime_error("not supported.");
    }
}