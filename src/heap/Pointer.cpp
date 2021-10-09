
#include "Heap.h"

#include <argc/types.h>
#include <stdexcept>

using namespace ascee;

template<typename T>
inline static
void copy(byte* dst, const byte* src) {
    *(T*) dst = *(T*) src;
}

static
void smartCopy(byte* dst, const byte* src, int32 size) {
    switch (size) {
        case 8:
            copy<int64>(dst, src);
            break;
        case 16:
            copy<int128>(dst, src);
            break;
        case 4:
            copy<int32>(dst, src);
            break;
        default:
            throw std::runtime_error("not supported.");
    }
}

void Heap::Pointer::readBlock(byte* dst, int32 size) {
    smartCopy(dst, heapPtr, size);
}

void Heap::Pointer::writeBlock(const byte* src, int32 size) {
    smartCopy(heapPtr, src, size);
}

