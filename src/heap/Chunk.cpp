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

#include <cstring>
#include "Chunk.h"

#define MAX_CHUNK_SIZE (32*1024)

using namespace ascee;
using namespace ascee::runtime::heap;

/// new chunks always have a size of zero. Their size usually should be changed through the pointer obtainable by
/// getSizePointer function.
/// The chunk will be zero initialized. This is important to make sure that smart contracts
/// behave deterministically and validators will agree on the result of executing a smart contact.
Chunk::Chunk(int capacity) : capacity(capacity), content(new byte[capacity]()) {
    if (capacity > MAX_CHUNK_SIZE) throw std::out_of_range("max chunk size exceeded");
}

int32 Chunk::getsize() const {
    return chunkSize;
}

bool Chunk::isTransient() const {
    return !bool(content);
}

Chunk::Pointer Chunk::getContentPointer(int32 offset) {
    return Pointer(content.get() + offset, content.get() + chunkSize);
}

void Chunk::reSize(int newSize) {
    if (newSize > MAX_CHUNK_SIZE || newSize < 0) throw std::out_of_range("invalid chunk size");

    if (newSize < chunkSize) {
        if (newSize == 0) {
            content = std::make_unique<byte[]>(0);
            capacity = 0;
        } else {
            memset(content.get() + newSize, 0, chunkSize - newSize);
        }
    }

    chunkSize = newSize;

    if (newSize <= capacity) return;

    auto newCapacity = std::min(MAX_CHUNK_SIZE, 2 * newSize);

    // memory must be zero initialized.
    auto* newContent = new byte[newCapacity]();
    memcpy(newContent, content.get(), chunkSize);
    content = std::unique_ptr<byte[]>(newContent);
    capacity = newCapacity;
}

bool Chunk::isValid(const byte* ptr) const {
    return ptr < content.get() + chunkSize || ptr == (byte*) &chunkSize;
}

void Chunk::setWritable(bool wr) {
    writable = wr;
}
