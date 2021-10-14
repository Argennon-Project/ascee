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

#ifndef ASCEE_ARGC_TYPES_H
#define ASCEE_ARGC_TYPES_H

#include <stdint.h> // NOLINT(modernize-deprecated-headers)

#ifdef __cplusplus
namespace ascee {

#endif
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
typedef __uint128_t full_id_t;
typedef struct String string_t;
typedef struct StringBuffer string_buffer;

typedef int (* dispatcher_ptr_t)(string_t request);

/// argc strings are not null-terminated. However, usually there is a null at the end. `length` is the number of
/// bytes without considering any null bytes at the end.
struct String {
    const char* content;
    int32 length;
};

struct StringBuffer {
    char* buffer;
    int32 maxSize;
    int32 end;
};

#define STRING(str) {.content = (str), .length = (sizeof (str) - 1)}
#define STRING_BUFFER(name, size) char __##name##_buf_[size]; string_buffer name = {__##name##_buf_, size, 0}

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

#ifdef __cplusplus

} // namespace ascee
#endif

#endif // ASCEE_ARGC_TYPES_H
