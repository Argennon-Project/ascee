
#include "Heap.h"

using namespace ascee;

Heap::Modifier* Heap::initSession(std_id_t calledApp) {
    auto* ret = new Modifier();
    ret->defineAccessBlock(Pointer(content + 5),
                           calledApp, 111, 5,
                           8, true);
    return ret;
}
