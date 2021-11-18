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

#ifndef ASCEE_ARGC_TYPES_INC
#define ASCEE_ARGC_TYPES_INC

#include <cstdint>
#include <util/StringBuffer.h>

namespace ascee {

/// int represents a signed integer with the most efficient size for the platform which MUST NOT be smaller than 32 bits.
typedef uint8_t byte;
typedef uint16_t uint16;
typedef int32_t int32;
typedef int64_t int64;
typedef __int128_t int128;
typedef double float64;
typedef __float128 float128;
typedef uint32_t short_id_t;
typedef uint64_t std_id_t;
typedef struct FullID full_id_t;


struct FullID {
    __int128_t id;

    FullID(__int128_t id) : id(id) {} // NOLINT(google-explicit-constructor)

    FullID(std_id_t high, std_id_t low) { id = __int128_t(high) << 64 | low; }

    operator __int128_t() const { return id; } // NOLINT(google-explicit-constructor)
};

/// argc strings are not null-terminated. However, usually there is a null at the end. `length` is the number of
/// bytes without considering any null bytes at the end.
using string_t = runtime::StringView;
template<int max_size>
using string_buffer = runtime::StringBuffer<max_size>;

typedef int (* dispatcher_ptr_t)(string_t request);

#define STRING(str) string_t(str)
#define STRING_BUFFER(name, size) string_buffer<size> name

#define RESPONSE_MAX_SIZE (2*1024)

/// HTTP status codes. new costume coded could be defined.
#define HTTP_OK 200
#define BAD_REQUEST 400
#define NOT_FOUND 404
/// The HTTP status code used when an app is out of gas, and its execution time is too long.
#define REQUEST_TIMEOUT 408
/// This HTTP status code indicates that the transaction has violated its predeclared resource caps.
#define PRECONDITION_FAILED 412
/// The HTTP status code used when a deferred called (spawned invocation) has failed.
#define FAILED_DEPENDENCY 424
#define INTERNAL_ERROR 500
/// This HTTP status code "usually" indicates that there is an infinite recursive loop in an app which causes an
/// stack overflow.
#define LOOP_DETECTED 508
#define INSUFFICIENT_STORAGE 507
/// This HTTP status is returned when the entrance lock is activated.
#define REENTRANCY_DETECTED 513
#define MAX_CALL_DEPTH_REACHED 514

} // namespace ascee

#endif // ASCEE_ARGC_TYPES_INC
