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

#ifndef ARGENNON_CHUNK_H
#define ARGENNON_CHUNK_H

#include <arg/primitives.h>
#include <arg/info.h>
#include <memory>
#include <atomic>
#include <mutex>
#include "util/crypto/DigestCalculator.h"

namespace argennon::ascee::runtime {


class Chunk {
public:
    static constexpr int maxAllowedCapacity = 64 * 1024;

    class Pointer {
    public:
        Pointer() = default;

        Pointer(byte* ptr, int32) : location(ptr) {}

        inline bool isNull() { return location == nullptr; }

        inline byte* get(int32_fast accessSize) {
            return location;
        }

    private:
        // We are going to have a lot of objects of this class in memory, so we try to keep it very lightweight.
        byte* const location = nullptr;
    };

    Chunk() = default;

    explicit Chunk(int32 capacity);

    Chunk(const Chunk&) = delete;

    explicit operator std::string() const;

    [[nodiscard]]
    int32 getsize() const;

    void setSize(int32 newSize);;

    /// This function should only be called at the start of block validation.
    bool reserveSpace(int32 newCapacity);

    /// This function should only be called at the end of block validation.
    bool shrinkSpace();

    [[nodiscard]]
    util::Digest calculateDigest() const { return {}; };

    [[nodiscard]]
    bool isWritable() const;;

    Pointer getContentPointer(int32 offset, int32 size);

    std::mutex& getContentMutex();

    const byte* applyDelta(const byte* delta, const byte* boundary);

    /// This is used to indicate that a chunk will not be modified in a block. Knowing that a chunk is not modified
    /// in the block helps in efficient calculation of commitments.
    Chunk* setWritable(bool writable);

private:
    std::unique_ptr<byte[]> content;
    // chunkSize must be atomic because there is a possibility for concurrent access, when for example the chunk is
    // going to be expanded and a request wants to load the chunk to read the start of the chunk.
    // In other words, when the chunkSize is going to be modified the scheduler may still allow some readers of
    // chunkSize to be run concurrently. but concurrent writes can not happen.
    std::atomic<int32> chunkSize = 0;
    int32 capacity = 0;
    bool writable = true;
    std::mutex contentMutex;

    void resize(int32 newCapacity);
};

} // namespace argennon::ascee::runtime::heap
#endif // ARGENNON_CHUNK_H
