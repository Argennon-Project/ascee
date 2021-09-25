//
// Created by aybehrouz on 9/22/21.
//

#include <stdio.h>
#include "HeapModifier.h"

using namespace ascee;

int64 HeapModifier::loadInt64(int32 offset) {
    printf("\n%d-->%d\n", context, offset);
}

void HeapModifier::openContext(std_id_t appID) {

}

void HeapModifier::closeContextNormally(std_id_t from, std_id_t to) {

}

void HeapModifier::closeContextAbruptly(std_id_t from, std_id_t to) {

}
