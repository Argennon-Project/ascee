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

#ifndef ASCEE_PRIMITIVES_H
#define ASCEE_PRIMITIVES_H

#include <cstdint>

namespace ascee {

/// int represents a signed integer with the most efficient size for the platform which MUST NOT be smaller than 32 bits.
typedef uint8_t byte;
typedef uint16_t uint16;
typedef int32_t int32;
typedef int_fast32_t int32_fast;
typedef int64_t int64;
typedef int_fast64_t int64_fast;
typedef __int128_t int128;
typedef double float64;
typedef __float128 float128;
typedef uint32_t short_id;
typedef uint64_t long_id;
typedef struct FullID full_id;

struct FullID {
    __int128_t id;

    FullID(__int128_t id) : id(id) {} // NOLINT(google-explicit-constructor)

    FullID(long_id up, long_id down) { id = __int128_t(up) << 64 | down; }

    operator __int128_t() const { return id; } // NOLINT(google-explicit-constructor)
};

struct Digest {
    int x = 0;
public:
    bool operator==(const Digest& rhs) const {
        return x == rhs.x;
    }

    bool operator!=(const Digest& rhs) const {
        return !(rhs == *this);
    }
};

} // namespace ascee
#endif // ASCEE_PRIMITIVES_H
