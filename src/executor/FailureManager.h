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

#ifndef ASCEE_FAILURE_MANAGER_H
#define ASCEE_FAILURE_MANAGER_H

#include <unordered_map>
#include <argc/types.h>

namespace ascee::runtime {

class FailureManager {
public:
    enum Reason {
        time, stack
    };

    typedef std::unordered_map<int_fast32_t, Reason> FailureMap;

    FailureManager() = default;

    explicit FailureManager(FailureMap failureList);

    int_fast32_t nextInvocation();

    void completeInvocation();

    int_fast64_t getExecTime(int_fast32_t, int_fast32_t gas);

    size_t getStackSize(int_fast32_t);

private:
    FailureMap failures;
    int callDepth = 0;
    int_fast32_t lastGeneratedID = 0;
};

} // namespace ascee::runtime
#endif // ASCEE_FAILURE_MANAGER_H
