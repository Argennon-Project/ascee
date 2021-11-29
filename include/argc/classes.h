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

inline static std::unordered_map<StatusCode, const char*> gReasonByCode{
        {StatusCode::not_found,          "Not Found"},
        {StatusCode::limit_violated,     "Declared Limits Violated"},
        {StatusCode::execution_timeout,  "Execution Timeout"},
        {StatusCode::internal_error,     "Internal Error"},
        {StatusCode::limit_exceeded,     "Resource Limit Reached"},
        {StatusCode::invalid_operation,  "Invalid Operation"},
        {StatusCode::arithmetic_error,   "Arithmetic Error"},
        {StatusCode::reentrancy_attempt, "Reentrancy Attempt"},
};


class execution_error : std::exception {
public:
    explicit execution_error(std::string msg, StatusCode code = StatusCode::internal_error) : msg(std::move(msg)),
                                                                                              code(code) {}

    [[nodiscard]] int errorCode() const { return (int) code; }

    [[nodiscard]] StatusCode statusCode() const { return code; }

    [[nodiscard]] std::string_view message() const { return msg; }

    template<int size>
    void toHttpResponse(runtime::StringBuffer<size>& response) const {
        response << "HTTP/1.1 " << errorCode() << " ";
        response << gReasonByCode.at(code) << "\r\n";
        response << "Content-Length: " << (int) msg.size() + 8 << "\r\n\r\n";
        response << "Error: " << message() << ".";
    }

private:
    const std::string msg;
    const StatusCode code;
};

} // namespace ascee
#endif // ASCEE_CLASSES_TYPES_H
