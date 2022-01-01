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

#ifndef ASCEE_FAILURE_MANAGER_H
#define ASCEE_FAILURE_MANAGER_H

#include <unordered_set>

namespace ascee::runtime {

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

} // namespace ascee::runtime
#endif // ASCEE_FAILURE_MANAGER_H
