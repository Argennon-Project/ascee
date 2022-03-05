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
#include "core/primitives.h"

namespace argennon::ascee::runtime {

class FailureManager {
public:
    typedef int32_fast InvocationID;

    FailureManager() = default;

    FailureManager(std::unordered_set<InvocationID> stackFailures,
                   std::unordered_set<InvocationID> cpuTimeFailures);

    int32_fast nextInvocation();

    void completeInvocation();

    int64_fast getExecTime(InvocationID id, int32_fast gas);

    size_t getStackSize(InvocationID id);

private:
    std::unordered_set<InvocationID> stackFailures;
    std::unordered_set<InvocationID> cpuTimeFailures;
    int callDepth = 0;
    InvocationID lastGeneratedID = 0;
};

} // namespace argennon::ascee::runtime
#endif // ARGENNON_FAILURE_MANAGER_H
