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
    class Pointer {
    public:
        Pointer() = default;

        explicit Pointer(byte* heapPtr, byte* lastValid) noexcept: heapPtr(heapPtr), boundary(lastValid) {}

        inline bool isNull() { return heapPtr == nullptr; }

        inline byte* get(int32 accessSize) {
            if (heapPtr + accessSize >= boundary) throw std::out_of_range("out of chunk");
            return heapPtr;
        }

    private:
        // We are going to have a lot of copies of this object. So we try to optimize its memory usage and avoid
        // storing any unnecessary data in this class.
        byte* const heapPtr = nullptr;
        byte* const boundary = nullptr;
    };

    Chunk() noexcept = default;

    explicit Chunk(int32 capacity);

    [[nodiscard]] int32 getsize() const;

    void reSize(int newSize);


    [[nodiscard]] bool isWritable() const {
        return writable;
    };

    [[nodiscard]] bool isTransient() const;

    Pointer getContentPointer(int32 offset);

    void setWritable(bool writable);

private:
    std::unique_ptr<byte[]> content;
    std::atomic<int32> chunkSize = 0;
    int32 capacity = 0;
    bool writable = false;

    bool isValid(const byte* ptr) const;
};

} // namespace ascee::runtime::heap
#endif // ASCEE_CHUNK_H
