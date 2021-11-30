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

#include <argc/classes.h>
#include "FailureManager.h"

#define MAX_CALL_DEPTH 16
#define DEFAULT_STACK_SIZE 2*1024*1024
#define FAIL_CHECK_STACK_SIZE 1024*1024
#define DEFAULT_GAS_COEFFICIENT 300000
#define FAIL_CHECK_GAS_COEFFICIENT 150000

using namespace ascee;
using namespace ascee::runtime;

int_fast32_t FailureManager::nextInvocation() {
    lastGeneratedID++;
    callDepth++;
    return lastGeneratedID;
}

void FailureManager::completeInvocation() {
    callDepth--;
}

int_fast64_t FailureManager::getExecTime(InvocationID id, int_fast32_t gas) {
    if (cpuTimeFailures.contains(id)) return FAIL_CHECK_GAS_COEFFICIENT * gas;
    return DEFAULT_GAS_COEFFICIENT * gas;
}

size_t FailureManager::getStackSize(InvocationID id) {
    if (callDepth > MAX_CALL_DEPTH) {
        throw AsceeException("max call depth reached", StatusCode::limit_exceeded);
    }
    if (stackFailures.contains(id)) return FAIL_CHECK_STACK_SIZE;
    return DEFAULT_STACK_SIZE;
}

FailureManager::FailureManager(std::unordered_set<InvocationID> stackFailures,
                               std::unordered_set<InvocationID> cpuTimeFailures)
        : stackFailures(std::move(stackFailures)), cpuTimeFailures(std::move(cpuTimeFailures)) {}
