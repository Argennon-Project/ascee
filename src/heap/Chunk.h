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
    static constexpr int maxCapacity = 64 * 1024;

    class Pointer {
    public:
        // We want this function to be thread-safe. That's why we let invalid pointers (pointers outside a chunk) be
        // created. We check these pointers later when they are accessed. We should also note that the size of the
        // chunk can be changed after a pointer is created.
        Pointer(Chunk* chunk, int32 offset);

        explicit Pointer(byte* ptr) : location(ptr) {}

        inline bool isNull() { return location == nullptr; }

        inline byte* get(int32 accessSize) {
            if (parent != nullptr && location + accessSize > parent->content.get() + parent->chunkSize) {
                throw std::out_of_range("out of chunk");
            }
            return location;
        }

        inline byte* getUnsafe() {
            return location;
        }

    private:
        // We are going to have a lot of copies of this object. So we try to optimize its memory usage and avoid
        // storing any unnecessary data in this class.
        Chunk* parent = nullptr;
        byte* const location = nullptr;
    };

    explicit Chunk(int32 capacity = initialCapacity);

    Chunk(const Chunk&) = delete;

    [[nodiscard]] int32 getsize() const;

    void setSize(int32 newSize);;

    bool reserveSpace(int32 size);;

    bool shrinkToFit();

    [[nodiscard]] bool isWritable() const {
        return writable;
    };

    [[nodiscard]] bool isTransient() const;

    Pointer getContentPointer(int32 offset);

    void setWritable(bool writable);

private:
    std::unique_ptr<byte[]> content;
    int32 chunkSize = 0;
    int32 capacity = 0;
    bool writable = true;

    void resize(int32 newCapacity);
};

} // namespace ascee::runtime::heap
#endif // ASCEE_CHUNK_H
