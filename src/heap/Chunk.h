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

#ifndef ASCEE_CHUNK_H
#define ASCEE_CHUNK_H

#include <argc/types.h>
#include <memory>

namespace ascee {

class Chunk {

public:
    class Pointer {
        friend class Chunk;

    private:
        byte* const heapPtr;

        explicit Pointer(byte* heapPtr) noexcept: heapPtr(heapPtr) {}

    public:
        template<typename T>
        inline
        T read() { return *(T*) heapPtr; }

        inline bool isNull() { return heapPtr == nullptr; }

        void readBlockTo(byte* dst, int32 size);

        void writeBlock(const byte* src, int32 size);
    };

private:
    std::unique_ptr<byte[]> content;
    int32 chunkSize = 0;
    int32 capacity = 0;

public:
    Chunk() noexcept = default;

    explicit Chunk(int32 size);

    int32 getsize() const;

    void expandSpace(int extra);

    bool isTransient() const;

    Pointer getSizePointer();

    Pointer getContentPointer(int32 offset);
};

} // namespace ascee
#endif // ASCEE_CHUNK_H
