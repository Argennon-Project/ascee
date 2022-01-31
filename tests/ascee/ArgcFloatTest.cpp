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

#include "gtest/gtest.h"
#include "argc/types.h"
#include "argc/functions.h"
#include "subtest.h"

using namespace argennon;
using namespace ascee::argc;


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
    struct {
        float64 a, b, c;
        int n = 0;
        bool wantErrSafe = false;
        bool wantErrExact = false;

        void test() {
            float64 second = n == 0 ? b : truncate_float64(b, n);
            if (wantErrExact) {
                EXPECT_THROW(exact_addf64(a, second), std::underflow_error);
            } else {
                EXPECT_DOUBLE_EQ(exact_addf64(a, second), c);
            }
            if (wantErrSafe) {
                EXPECT_THROW(safe_addf64(a, second), std::underflow_error);
            } else {
                EXPECT_DOUBLE_EQ(safe_addf64(a, second), c);
            }
        }
    } testCase;

    testCase = {
            .a = 1000000,
            .b = 1.3756,
            .c = 1000001.3756,
            .wantErrExact = true
    };
    SUB_TEST("", testCase);

    testCase = {
            .a = 1000000,
            .b = -1.37,
            .c = 999998.63,
            .wantErrExact = true
    };
    SUB_TEST("", testCase);

    testCase = {
            .a = 1000000000000,
            .b = 1.375,
            .c = 1000000000001.375,
            .n = 12
    };
    SUB_TEST("", testCase);

    testCase = {
            .a = -1000000000000,
            .b = 1.375,
            .c = -999999999998.625,
            .n = 12
    };
    SUB_TEST("", testCase);

    // 0.2 has an infinite binary expansion
    testCase = {
            .a = 100000,
            .b = 0.2,
            .c = 100000.2,
            .wantErrExact = true
    };
    SUB_TEST("", testCase);

    testCase = {
            .a = 1000000,
            .b = 0.2,
            .wantErrSafe = true,
            .wantErrExact = true
    };
    SUB_TEST("", testCase);

    testCase = {
            .a = -1000000,
            .b = 0.2,
            .wantErrSafe = true,
            .wantErrExact = true
    };
    SUB_TEST("", testCase);

    testCase = {
            .a = 1000000,
            .b = -0.2,
            .wantErrSafe = true,
            .wantErrExact = true
    };
    SUB_TEST("", testCase);

    testCase = {
            .a = 1000000000000000,
            .b = 0.125,
            .wantErrSafe = true,
            .wantErrExact = true
    };
    SUB_TEST("", testCase);

    testCase = {
            .a = -1000000000000000,
            .b = 0.125,
            .wantErrSafe = true,
            .wantErrExact = true
    };
    SUB_TEST("", testCase);

    testCase = {
            .a = 1000000000000000,
            .b = -0.125,
            .wantErrSafe = true,
            .wantErrExact = true
    };
    SUB_TEST("", testCase);

    testCase = {
            .a = -1000000000000000,
            .b = -0.125,
            .wantErrSafe = true,
            .wantErrExact = true
    };
    SUB_TEST("", testCase);

    testCase = {
            .a = 0.1,
            .b = 0.2,
            .c = 0.3
    };
    SUB_TEST("", testCase);

    testCase = {
            .a = 100000000000,
            .b = 0.2,
            .c = 100000000000.1999817,
            .n = 15,
    };
    SUB_TEST("", testCase);

    testCase = {
            .a = 1000000000000,
            .b = 0.2,
            .c = 1000000000000.1999817,
            .n = 15,
            .wantErrSafe = true,
            .wantErrExact = true
    };
    SUB_TEST("", testCase);

    uint64_t s_0 = 10000000;
    uint64_t r_0 = 200000000000;
    float64 s = s_0, r = r_0;
    float64 t = 0.2;

    long long n = 10000000;
    for (int i = 0; i < n; ++i) {
        s -= t;
        r += t;
    }

    printf("%lf %lf\n", s, r);
    printf("lost: %lf \n", (s_0 - s) + (r_0 - r));

    s = s_0, r = r_0;
    float64 tr = truncate_float64(t, 15);
    for (int i = 0; i < n; ++i) {
        s = safe_addf64(s, -tr);
        r = safe_addf64(r, tr);
    }
    EXPECT_EQ((s_0 - s) + (r_0 - r), 0);

    s = s_0, r = r_0;
    for (int i = 0; i < n; ++i) {
        s = exact_addf64(s, -tr);
        r = exact_addf64(r, tr);
    }
    EXPECT_EQ((s_0 - s) + (r_0 - r), 0);
    printf("%lf %lf\n", s, r);
    printf("lost: %lf \n", (s_0 - s) + (r_0 - r));
}


