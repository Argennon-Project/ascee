
#include "argc/types.h"
#include <iostream>

#ifndef ASCEE_HEAP_MODIFIER_H
#define ASCEE_HEAP_MODIFIER_H

namespace ascee {

class HeapModifier {
    friend class Heap;

private:
    int context;

    HeapModifier(int c) { context = c; }

public:
    int64_t loadInt64(int32_t offset);

    void storeInt64(int64_t value, int32_t offset);

    void loadChunk(argc::short_id_t chunkID);

    void openContext(argc::std_id_t appID);

    void closeContextNormally(argc::std_id_t from, argc::std_id_t to);

    void closeContextAbruptly(argc::std_id_t from, argc::std_id_t to);

    void save();

    void restore();

    void discard();

    ~HeapModifier() {
        std::cout << "mod:dest" << std::endl;
    }
};

} // namespace ascee
#endif // ASCEE_HEAP_MODIFIER_H

