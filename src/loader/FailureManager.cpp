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

#include "FailureManager.h"

#include <utility>

#define MAX_DEPTH 16
#define DEFAULT_STACK_SIZE 2*1024*1024
#define FAIL_CHECK_STACK_SIZE 1024*1024
#define DEFAULT_GAS_COEFFICIENT 300000
#define FAIL_CHECK_GAS_COEFFICIENT 150000

using namespace ascee;
using std::unordered_map;

void FailureManager::nextInvocation() {
    invocationID++;
    callDepth++;
}

void FailureManager::completeInvocation() {
    callDepth--;
}

int64_t FailureManager::getExecTime(int64_t gas) {
    try {
        auto reason = failureList.at(invocationID);
        if (reason == time) return FAIL_CHECK_GAS_COEFFICIENT * gas;
    } catch (const std::out_of_range&) {}

    return DEFAULT_GAS_COEFFICIENT * gas;
}

size_t FailureManager::getStackSize() {
    if (callDepth > MAX_DEPTH) throw std::overflow_error("max call depth reached");
    try {
        auto reason = failureList.at(invocationID);
        if (reason == stack) return FAIL_CHECK_STACK_SIZE;
    } catch (const std::out_of_range&) {}

    return DEFAULT_STACK_SIZE;
}

FailureManager::FailureManager(unordered_map<int32_t, Reason> failureList) : failureList(std::move(failureList)) {}
