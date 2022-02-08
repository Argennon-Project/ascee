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

namespace argennon {

/// int represents a signed integer with the most efficient size for the platform which MUST NOT be smaller than 32 bits.
typedef uint8_t byte;
typedef uint16_t uint16;
typedef int16_t int16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int_fast32_t int32_fast;
typedef int64_t int64;
typedef int_fast64_t int64_fast;
typedef __int128_t int128;
typedef double float64;
//typedef __float128 float128;

typedef uint32_t short_id;
typedef struct LongID long_id;
typedef struct LongLongID long_long_id;
typedef struct FullID full_id;

} // namespace argennon
#endif // ASCEE_PRIMITIVES_H
