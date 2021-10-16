// Copyright (c) 2021 aybehrouz <behrouz_ayati@yahoo.com>. All rights reserved.
// This file is part of the C++ implementation of the Argennon smart contract
// Execution Environment (AscEE).
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option) any
// later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License
// for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

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
            throw std::runtime_error("not implemented.");
    }
}

void Heap::Pointer::readBlockTo(byte* dst, int32 size) {
    smartCopy(dst, heapPtr, size);
}

void Heap::Pointer::writeBlock(const byte* src, int32 size) {
    smartCopy(heapPtr, src, size);
}

