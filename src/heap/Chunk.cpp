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

#define MAX_CHUNK_SIZE 32*1024

using namespace ascee;

/// new chunks always have a size of zero. Their size usually should be changed through the pointer obtainable by
/// getSizePointer function.
/// The chunk will be zero initialized. This is important to make sure that smart contracts
/// behave deterministically and validators will agree on the result of executing a smart contact.
Chunk::Chunk(int size) : capacity(size), content(new byte[size]()) {
    if (size > MAX_CHUNK_SIZE) throw std::out_of_range("max chunk size exceeded");
}

int32 Chunk::getsize() const {
    return chunkSize;
}

bool Chunk::isTransient() const {
    return !bool(content);
}

Chunk::Pointer Chunk::getContentPointer(int32 offset) {
    return Pointer(content.get() + offset);
}

Chunk::Pointer Chunk::getSizePointer() {
    return Pointer(reinterpret_cast<byte*>(&chunkSize));
}

void Chunk::expandSpace(int extra) {
    if (chunkSize + extra <= capacity) return;

    if (chunkSize + extra > MAX_CHUNK_SIZE) throw std::out_of_range("can't expand beyond max chunk size");

    auto newCapacity = std::min(MAX_CHUNK_SIZE, 2 * (chunkSize + extra));

    // memory must be zero initialized.
    auto* newContent = new byte[newCapacity]();
    memcpy(newContent, content.get(), chunkSize);
    content = std::unique_ptr<byte[]>(newContent);
    capacity = newCapacity;
}
