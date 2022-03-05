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

#ifndef ARGENNON_ARGC_TYPES_H
#define ARGENNON_ARGC_TYPES_H

#include <core/primitives.h>
#include "StringBuffer.h"
#include "util/crypto/Keys.h"

namespace argennon::ascee {


/// argc strings are not null-terminated. However, usually there is a null at the end. `length` is the number of
/// bytes without considering any null bytes at the end.
using string_view_c = runtime::StringView;
template<int max_size>
using string_buffer_c = runtime::StringBuffer<max_size>;
using response_buffer_c = runtime::StringBuffer<8 * 1024>;
template<typename T, int size>
using array_c = util::StaticArray<T, size>;

using signature_c = util::Signature;
using publickey_c = util::PublicKey;

/// We use a fixed size buffer for messages, instead of allowing smart contracts to choose the buffer size.
/// I believe letting smart contracts choose this size can introduce undesirable coupling of smart contracts
/// to the suffix size of messages, which is appended by the signature verification functions.
using message_c = runtime::StringBuffer<2 * 1024>;

/// HTTP response status codes
enum class StatusCode : int {
    bad_request = 400,
    forbidden = 403,
    not_found = 404,
    /// This HTTP status code indicates that the transaction has violated its predeclared resource limits.
    limit_violated = 420,
    /// Indicates that finishing the execution of the application was not possible in the transaction's specified time
    execution_timeout = 421,
    /// Indicates a general error
    internal_error = 500,
    /// Indicates that a protocol defined limit for a resource was exceeded by the smart contract.
    limit_exceeded = 520,
    /// Indicates that a smart contract tried to perform an operation that was not valid
    invalid_operation = 521,
    ///
    arithmetic_error = 522,
    reentrancy_attempt = 523,
    memory_fault = 524,
    out_of_range = 525,
};

constexpr const char* gReasonByStatusCode(StatusCode code) {
    switch (code) {
        case StatusCode::bad_request:
            return "Bad Request";
        case StatusCode::not_found:
            return "Not Found";
        case StatusCode::limit_violated:
            return "Declared Limits Violated";
        case StatusCode::execution_timeout:
            return "Execution Timeout";
        case StatusCode::internal_error:
            return "Internal Error";
        case StatusCode::limit_exceeded:
            return "Resource Limit Reached";
        case StatusCode::invalid_operation:
            return "Invalid Operation";
        case StatusCode::arithmetic_error:
            return "Arithmetic Error";
        case StatusCode::reentrancy_attempt:
            return "Reentrancy Attempt";
        case StatusCode::forbidden:
            return "Forbidden";
        case StatusCode::memory_fault:
            return "Internal Memory Fault";
        case StatusCode::out_of_range:
            return "Out of Range Access";

    }
    return "Unknown Reason";
}

class AsceeError : public std::exception {
public:
    ~AsceeError() override = default;

    explicit AsceeError(
            std::string msg,
            StatusCode code = StatusCode::internal_error,
            std::string thrower = ""
    ) noexcept: msg(std::move(msg)),
                code(code), thrower(std::move(thrower)),
                message(this->thrower.empty() ? this->msg : "[" + this->thrower + "]-> " + this->msg) {}

    [[nodiscard]] int errorCode() const { return (int) code; }

    [[nodiscard]] const char* what() const noexcept override { return message.c_str(); }

    const std::string thrower;
    const std::string msg;
    const std::string message;
    const StatusCode code;
};

typedef int (* DispatcherPointer)(response_buffer_c& response, string_view_c request);

#define STRING(name, str) char __##name##_buf__[] = str; string_view_c name(__##name##_buf__)
#define STRING_BUFFER(name, size) string_buffer_c<size> name
#define dispatcher extern "C" int dispatcher(response_buffer_c& response, string_view_c request)

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
#endif // ARGENNON_ARGC_TYPES_H
