
#include "../../include/argc/types.h"
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
    int64 loadInt64(int32 offset);

    void storeInt64(int64 value, int32 offset);

    void loadChunk(short_id_t chunkID);

    void openContext(std_id_t appID);

    void closeContextNormally(std_id_t from, std_id_t to);

    void closeContextAbruptly(std_id_t from, std_id_t to);

    void save();

    void restore();

    void discard();

    ~HeapModifier() {
        std::cout << "mod:dest" << std::endl;
    }
};

} // namespace ascee
#endif // ASCEE_HEAP_MODIFIER_H

