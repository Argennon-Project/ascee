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
#include <sstream>
#include <cassert>
#include "Chunk.h"
#include "util/PrefixTrie.hpp"

using namespace argennon;
using namespace argennon::ascee::runtime;
using namespace util;
using std::unique_ptr, std::make_unique;

/// new chunks always have a size of zero. Their size usually should be changed by using reSize() function.
/// The chunk will be zero initialized. This is important to make sure that smart contracts
/// behave deterministically and validators will agree on the result of executing a smart contact.
Chunk::Chunk(uint capacity) : chunkSize(0), capacity(capacity) {
    assert(capacity <= maxAllowedCapacity);
    content = make_unique<byte[]>(capacity);
    // arrays created by make_unique are value initialized.
}

uint32 Chunk::getsize() const {
    return chunkSize;
}

Chunk::Pointer Chunk::getContentPointer(uint32 offset, uint32 size) {
    if (int64(offset) + int64(size) > int64(capacity)) throw std::out_of_range("out of allocated memory range");
    return {content.get() + offset, size};
}

Chunk* Chunk::setWritable(bool wr) {
    writable = wr;
    return this;
}

void Chunk::setSize(uint32 newSize) {
    assert(newSize <= capacity);

    // Based on specs offsets beyond chunkSize must be zero initialised at the start of every execution session.
    if (newSize < chunkSize) {
        memset(content.get() + newSize, 0, chunkSize - newSize);
    }

    chunkSize = newSize;
}

void Chunk::resize(uint32 newCapacity) {
    // we need to zero initialize the empty part of the memory.
    auto* newContent = new byte[newCapacity];
    memcpy(newContent, content.get(), chunkSize);
    if (newCapacity > chunkSize) memset(newContent + chunkSize, 0, newCapacity - chunkSize);
    content.reset(newContent);
    capacity = newCapacity;
}

bool Chunk::reserveSpace(uint32 newCapacity) {
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

/**
 * @format: [ chunkSize (offsetDiff dataSize data)* 0 ]
 * @format @p offsetDiff is the difference between the end of the last data block and the start of the current
 * data block + 1.
 * @format @p 0 indicates the end of the delta
 * @format @p chunkSize will be XORed with the current chunk size.
 * @format @p offsetDiff and dataSize are used without XORing.
 * @format data is the data to be copied in the chunk
 * @format Type of all numbers are varSize encoded with @p var_size_trie_g PrefixTrie.
 *
 * @param delta
 * @return the updated delta pointer in such a way that it points to the unconsumed part of the delta array.
 */
void Chunk::applyDelta(const byte*& delta, const byte* boundary) {
    if (delta >= boundary) return;

    auto size = chunkSize ^ (int32) var_uint_trie_g.decodeVarUInt(&delta, boundary);
    reserveSpace(size);

    int32_fast offset = 0;
    while (delta < boundary) {
        auto diff = var_uint_trie_g.decodeVarUInt(&delta, boundary);
        if (diff == 0) break;
        offset += diff - 1;
        auto blockSize = var_uint_trie_g.decodeVarUInt(&delta, boundary);
        if (offset + blockSize > size) {
            // For being able to remove deltas we need this. Do not change it!
            if (offset < size) blockSize = size - offset;
            else break;
        }

        memcpy(content.get() + offset, delta, blockSize);
        delta += blockSize;
        offset += blockSize;
    }
    chunkSize = size;
    // important!
    shrinkSpace();
}

bool Chunk::isWritable() const {
    return writable;
}

std::mutex& Chunk::getContentMutex() {
    return contentMutex;
}

Chunk::operator std::string() const {
    using std::stringstream, std::to_string;
    stringstream result;
    result << "size: " << chunkSize << ", ";
    result << "capacity: " << capacity << ", ";
    result << "content: 0x[ ";
    for (int i = 0; i < chunkSize; ++i) {
        result << std::hex << (int) content[i] << " ";
    }
    result << "]";
    return result.str();
}
