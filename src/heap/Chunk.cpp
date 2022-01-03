// Copyright (c) 2021-2022 aybehrouz <behrouz_ayati@yahoo.com>. All rights
// reserved. This file is part of the C++ implementation of the Argennon smart
// contract Execution Environment (AscEE).
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

#include <cstring>
#include <cassert>
#include "Chunk.h"

using namespace ascee;
using namespace ascee::runtime::heap;
using std::unique_ptr, std::make_unique;

/// new chunks always have a size of zero. Their size usually should be changed by using reSize() function.
/// The chunk will be zero initialized. This is important to make sure that smart contracts
/// behave deterministically and validators will agree on the result of executing a smart contact.
Chunk::Chunk(int capacity) : capacity(capacity), chunkSize(0) {
    if (capacity > maxCapacity) throw std::out_of_range("max chunk size exceeded");
    content = make_unique<byte[]>(capacity);
    // arrays created by make_unique are value initialized.
}

int32 Chunk::getsize() const {
    return chunkSize;
}

bool Chunk::isTransient() const {
    return !bool(content);
}

Chunk::Pointer Chunk::getContentPointer(int32 offset) {
    return {this, offset};
}

void Chunk::setWritable(bool wr) {
    writable = wr;
}

void Chunk::setSize(int32 newSize) {
    assert(newSize <= capacity && newSize >= 0);
    chunkSize = newSize;
    // We do not zero memory when the chunk is shrinking, since locations outside the chunk can not be accessed
    // in the current implementation.
}

void Chunk::resize(int32 newCapacity) {
    // memory must be zero initialized only when we are expanding the chunk
    auto* newContent = newCapacity > chunkSize ? new byte[newCapacity]() : new byte[newCapacity];
    memcpy(newContent, content.get(), chunkSize);
    content.reset(newContent);
    capacity = newCapacity;
}

bool Chunk::reserveSpace(int32 size) {
    assert(size <= maxCapacity && size >= 0);

    if (size <= capacity) return false;

    resize(std::min(maxCapacity, 2 * size));
    return true;
}

bool Chunk::shrinkToFit() {
    if (chunkSize == capacity) return false;
    resize(chunkSize);
    return true;
}

Chunk::Pointer::Pointer(Chunk* chunk, int32 offset) : parent(chunk),
                                                      location(chunk->content.get() + offset) {
    assert(offset >= 0);
}
