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

#ifndef ASCEE_TYPES_INC
#define ASCEE_TYPES_INC

#include "primitives.h"
#include "classes.h"

namespace ascee {

typedef int (* dispatcher_ptr)(string_c request);

#define STRING(str) string_c(str)
#define STRING_BUFFER(name, size) string_buffer_c<size> name

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

#endif // ASCEE_TYPES_INC
