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

#include "gtest/gtest.h"
#include "argc/types.h"
#include "argc/functions.h"

using namespace ascee;
using namespace argc;


TEST(ArgcFloatDeathTest, Truncate) {
    float64 f = 2.9999999;

    int n = 14;
    float64 e = 1.0 / (1 << n);
    float64 truncated = truncate_float64(f, n);
    printf("tr:%lf\n", truncated);
    EXPECT_LE(abs(truncated - f), e);
    EXPECT_GE(abs(truncated - f), 0.5 * e);

    n = 1;
    e = 1.0 / (1 << n);
    truncated = truncate_float64(f, n);
    printf("tr:%lf\n", truncated);
    EXPECT_LE(abs(truncated - f), e);
    EXPECT_GE(abs(truncated - f), 0.5 * e);


    n = 5;
    e = 1.0 / (1 << n);
    truncated = truncate_float64(f, n);
    printf("tr:%lf\n", truncated);
    EXPECT_LE(abs(truncated - f), e);
    EXPECT_GE(abs(truncated - f), 0.5 * e);


    n = 50;
    e = 1.0 / (1 << n);
    truncated = truncate_float64(f, n);
    printf("tr:%lf\n", truncated);
    EXPECT_DOUBLE_EQ(truncated, f);

    n = 0;
    e = 1.0 / (1 << n);
    truncated = truncate_float64(f, n);
    EXPECT_EQ(truncated, 2);
}


TEST(ArgcFloatDeathTest, SafeAdd) {
    EXPECT_DOUBLE_EQ(safe_addf64(1000000, 1.3756), 1000001.3756);

    EXPECT_THROW(safe_addf64(10000000, 1.3756), std::underflow_error);

    EXPECT_DOUBLE_EQ(safe_addf64(1000000, -1.37), 999998.63);

    EXPECT_THROW(safe_addf64(10000000, -1.37), std::underflow_error);

    EXPECT_DOUBLE_EQ(safe_addf64(1000000000000, truncate_float64(1.375, 12)), 1000000000001.375);
}


