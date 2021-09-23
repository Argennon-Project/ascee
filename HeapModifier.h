//
// Created by aybehrouz on 9/22/21.
//

#include "argctypes.h"
#include <iostream>

#ifndef ASCEE_HEAPMODIFIER_H
#define ASCEE_HEAPMODIFIER_H

class HeapModifier {
    friend class Heap;
private:
    int context;

    HeapModifier(int c) { context = c; }

public:
    int64 loadInt64(int32 offset);

    void storeInt64(int64 value, int32 offset);

    void loadChunk(short_id_t chunkID);

    void changeContext(std_id_t appID);

    void save();

    void restore();

    void discard();

    void saveCheckPoint();

    void RestoreCheckPoint();
    void DiscardCheckPoint();

    ~HeapModifier() {
        std::cout << "mod:dest" << std::endl;
    }
};

#endif //ASCEE_HEAPMODIFIER_H

