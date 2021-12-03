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


#ifndef ASCEE_CLASSES_TYPES_H
#define ASCEE_CLASSES_TYPES_H

#include <util/StringBuffer.h>
#include <crypto/Keys.h>

#include <utility>
#include "primitives.h"

namespace ascee {

/// argc strings are not null-terminated. However, usually there is a null at the end. `length` is the number of
/// bytes without considering any null bytes at the end.
using string_c = runtime::StringView;
template<int max_size>
using string_buffer_c = runtime::StringBuffer<max_size>;
using signature_c = runtime::Signature;
using publickey_c = runtime::PublicKey;

/// We use a fixed size buffer for messages, instead of allowing smart contracts to choose the buffer size.
/// I believe letting smart contracts choose this size can introduce undesirable coupling of smart contracts
/// to the suffix size of the messages, which is appended by the signature verification functions.
using message_c = runtime::StringBuffer<2 * 1024>;

/// HTTP response status codes
enum class StatusCode {
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
};

inline const char* gReasonByStatusCode(StatusCode code) {
    switch (code) {
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
    }
    return "Unknown Reason";
}

class AsceeException : public std::exception {
public:
    explicit AsceeException(
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


} // namespace ascee
#endif // ASCEE_CLASSES_TYPES_H
