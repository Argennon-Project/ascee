//
// Created by aybehrouz on 9/22/21.
//

#include <stdio.h>
#include "HeapModifier.h"

using namespace ascee;

int64_t HeapModifier::loadInt64(int32_t offset) {
    printf("\n%d-->%d\n", context, offset);
}

void HeapModifier::openContext(argc::std_id_t appID) {

}

void HeapModifier::closeContextNormally(argc::std_id_t from, argc::std_id_t to) {

}

void HeapModifier::closeContextAbruptly(argc::std_id_t from, argc::std_id_t to) {

}
