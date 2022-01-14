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

#ifndef ARGENNON_FAILURE_MANAGER_H
#define ARGENNON_FAILURE_MANAGER_H

#include <unordered_set>
#include "argc/types.h"

namespace argennon::ascee::runtime {

class FailureManager {
public:
    typedef int_fast32_t InvocationID;

    FailureManager() = default;

    FailureManager(std::unordered_set<InvocationID> stackFailures,
                   std::unordered_set<InvocationID> cpuTimeFailures);

    int_fast32_t nextInvocation();

    void completeInvocation();

    int_fast64_t getExecTime(InvocationID id, int_fast32_t gas);

    size_t getStackSize(InvocationID id);

private:
    std::unordered_set<InvocationID> stackFailures;
    std::unordered_set<InvocationID> cpuTimeFailures;
    int callDepth = 0;
    InvocationID lastGeneratedID = 0;
};

class ApplicationError : public std::exception {
public:
    explicit ApplicationError(
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


} // namespace argennon::ascee::runtime
#endif // ARGENNON_FAILURE_MANAGER_H
