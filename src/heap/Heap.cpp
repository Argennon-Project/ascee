
#include "Heap.h"

using namespace ascee;

static
bool isLittleEndian() {
    char buf[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

    int128 x = *(int128*) buf;
    auto xl = (int64) x;
    auto xh = int64(x >> 64);
    if (xl != 0x807060504030201) return false;
    if (xh != 0xf0e0d0c0b0a09) return false;
    return true;
}

Heap::Modifier* Heap::initSession(std_id_t calledApp) {
    auto* ret = new Modifier();
    ret->defineAccessBlock(Pointer(content + 5),
                           calledApp, short_id_t(111), 5,
                           8, false);
    return ret;
}

Heap::Heap() {
    if (!isLittleEndian()) throw std::runtime_error("platform not supported");
}
