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
    assert(capacity <= maxAllowedCapacity);
    content = make_unique<byte[]>(capacity);
    // arrays created by make_unique are value initialized.
}

int32 Chunk::getsize() const {
    return chunkSize;
}

Chunk::Pointer Chunk::getContentPointer(int32 offset, int32 size) {
    assert(offset >= 0 && size >= 0);
    if (offset + size > capacity) throw std::out_of_range("out of allocated memory range");
    return {content.get() + offset, size};
}

Chunk* Chunk::setWritable(bool wr) {
    writable = wr;
    return this;
}

void Chunk::setSize(int32 newSize) {
    assert(newSize <= capacity && newSize >= 0);

    // Based on specs offsets beyond chunkSize must be zero initialised at the start of every execution session.
    if (newSize < chunkSize) {
        memset(content.get() + newSize, 0, chunkSize - newSize);
    }

    chunkSize = newSize;
}

void Chunk::resize(int32 newCapacity) {
    // zero initialization is needed only when we are expanding the chunk
    auto* newContent = newCapacity > chunkSize ? new byte[newCapacity]() : new byte[newCapacity];
    if (chunkSize > 0) memcpy(newContent, content.get(), chunkSize);
    content.reset(newContent);
    capacity = newCapacity;
}

bool Chunk::reserveSpace(int32 newCapacity) {
    assert(newCapacity <= maxAllowedCapacity && newCapacity >= 0);

    if (newCapacity <= capacity) return false;

    resize(newCapacity);
    return true;
}

bool Chunk::shrinkToFit() {
    if (chunkSize == capacity) return false;
    resize(chunkSize);
    return true;
}
