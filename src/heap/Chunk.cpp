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
#include "util/IdentifierTrie.h"

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

bool Chunk::shrinkSpace() {
    if (chunkSize == capacity) return false;
    resize(chunkSize);
    return true;
}

ascee::runtime::IdentifierTrie<uint32_t, 4> gVarSizeTrie({0xd0, 0xf000, 0xfc0000, 0xffffff00});

static
int32_fast readVarSize(const byte*& readPtr, const byte* boundary) {
    auto len = 0;
    auto size = gVarSizeTrie.decodeVarUInt(readPtr, &len, int(boundary - readPtr));
    readPtr += len;
    return int32_fast(size);
}

/// computes x[0:size - 1] ^= a[0:size - 1]
void fastXOR(byte* x, const byte* a, int32_fast size) {
    int32_fast count = size / int32_fast(sizeof(int64_fast));
    auto x_fast = (int64_fast*) x;
    auto a_fast = (int64_fast*) a;

    for (int32_fast i = 0; i < count; ++i) {
        x_fast[i] ^= a_fast[i];
    }

    for (int32_fast i = count * int32_fast(sizeof(int64_fast)); i < size; ++i) {
        x[i] ^= a[i];
    }
}

void Chunk::applyDeltaReversible(const byte* delta, int32_fast len) {
    auto boundary = delta + len;

    auto size = chunkSize ^ (int32) readVarSize(delta, boundary);
    reserveSpace(size);

    int32_fast offset = 0;
    while (delta < boundary) {
        offset += readVarSize(delta, boundary);
        auto blockSize = readVarSize(delta, boundary);
        if (offset + blockSize > size) {
            if (offset < size) blockSize = size - offset;
            else break;
        }

        fastXOR(content.get() + offset, delta, blockSize);
        delta += blockSize;
        offset += blockSize;
    }
    chunkSize = size;
    updateDigest();
    // Still we need to call shrinkSpace() to complete the operation, but if we do it here the delta can not be removed
    // later.
}

void Chunk::applyDelta(const Digest& expectedDigest, const byte* delta, int32_fast len) {
    applyDeltaReversible(delta, len);
    if (expectedDigest != digest) {
        // first we remove incorrect delta.
        applyDeltaReversible(delta, len);
        shrinkSpace();
        throw std::invalid_argument("incorrect chunk delta");
    }
    // important!
    shrinkSpace();
}

void Chunk::updateDigest() {

}

bool Chunk::isWritable() const {
    return writable;
}

std::mutex& Chunk::getContentMutex() {
    return contentMutex;
}

Chunk::operator std::string() const {
    using std::string, std::to_string;
    string result;
    result += "size: " + to_string(chunkSize) + ", ";
    result += "capacity: " + to_string(capacity) + ", ";
    result += "content: [ ";
    for (int i = 0; i < chunkSize; ++i) {
        result += to_string(content[i]) + " ";
    }
    result += "]";
    return result;
}

const Digest& Chunk::getDigest() const {
    return digest;
}
