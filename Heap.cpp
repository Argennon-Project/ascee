//
// Created by aybehrouz on 9/22/21.
//

#include "Heap.h"

HeapModifier* Heap::setupSession(int temp)
{
    return new HeapModifier(temp);
}