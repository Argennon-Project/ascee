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

#ifndef ASCEE_CHUNK_H
#define ASCEE_CHUNK_H

#include <argc/types.h>
#include <memory>
#include <atomic>

namespace ascee::runtime::heap {


class Chunk {
public:
    static constexpr int initialCapacity = 32;
    static constexpr int maxAllowedCapacity = 64 * 1024;

    class Pointer {
    public:
        Pointer(byte* ptr, int32) : location(ptr) {}

        inline bool isNull() { return location == nullptr; }

        inline byte* get(int32 accessSize) {
            return location;
        }

    private:
        byte* const location = nullptr;
    };

    explicit Chunk(int32 capacity = initialCapacity);

    Chunk(const Chunk&) = delete;

    [[nodiscard]] int32 getsize() const;

    void setSize(int32 newSize);;

    /// This function should only be called at the start of block validation.
    bool reserveSpace(int32 newCapacity);

    /// This function should only be called at the end of block validation.
    bool shrinkToFit();

    [[nodiscard]] bool isWritable() const {
        return writable;
    };

    Pointer getContentPointer(int32 offset, int32 size);

    /// This is used to indicate that a chunk will not be modified in a block. Knowing that a chunk is not modified
    /// in the block helps in efficient calculation of commitments.
    void setWritable(bool writable);

private:
    std::unique_ptr<byte[]> content;
    // chunkSize must be atomic because there is a possibility for concurrent access, when for example the chunk is
    // going to be expanded and a request wants to load the chunk to read the start of the chunk.
    // In other words, when the chunkSize is going to be modified the scheduler may still allow some readers of
    // chunkSize to be run concurrently. but concurrent writes can not happen.
    std::atomic<int32> chunkSize = 0;
    int32 capacity = 0;
    bool writable = true;

    void resize(int32 newCapacity);
};

} // namespace ascee::runtime::heap
#endif // ASCEE_CHUNK_H
