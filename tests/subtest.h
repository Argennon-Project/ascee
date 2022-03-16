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

#ifndef ASCEE_SUBTEST_H
#define ASCEE_SUBTEST_H

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <string>

#define SUB_TEST(name, variable)  { SCOPED_TRACE(std::string(name).append("\n\n")); (variable).test(); }


#define BENCHMARK_ONCE(statements, name) \
auto start = std::chrono::steady_clock::now(); \
statements; \
auto end = std::chrono::steady_clock::now(); \
std::chrono::duration<double> elapsed_seconds = end - start; \
std::cout << (name) << ": " << elapsed_seconds.count() << " seconds\n";

#define BENCHMARK(statements, runs, name) { \
auto start = std::chrono::steady_clock::now(); \
for (int i = 0; i < (runs); ++i) { \
    statements; \
} \
auto end = std::chrono::steady_clock::now(); \
std::chrono::duration<double> elapsed_seconds = end - start; \
std::cout << (name) << ": " << elapsed_seconds.count() << " seconds\n"; }

#endif // ASCEE_SUBTEST_H
