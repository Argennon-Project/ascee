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

#include "subtest.h"
#include "argc/types.h"
#include "argc/functions.h"

using namespace argennon;
using namespace ascee;
using namespace argc;
using namespace std;

enum AppendType {
    STR, INT64
};

template<int s = 0>
struct AppendTestCase {
    string_buffer_c<s> buf;

    int64 int64ToAppend = 0;
    string_view strToAppend, wantResult;
    bool wantError = false;
    AppendType appendType = STR;

    void test() {
        if (wantError) {
            if (appendType == STR)
                EXPECT_THROW(
                        argc::append_str(buf, string_view_c(strToAppend)),
                        std::out_of_range
                );
            else if (appendType == INT64)
                EXPECT_THROW(
                        argc::append_int64(buf, int64ToAppend),
                        std::out_of_range
                );
            return;
        }
        if (appendType == STR) argc::append_str(buf, string_view_c(strToAppend));
        else if (appendType == INT64) argc::append_int64(buf, int64ToAppend);
        EXPECT_EQ(string_view_c(buf), wantResult);
    }
};


TEST(ArgcStringTest, Append) {

    AppendTestCase<10> testCase;

    testCase.strToAppend = {"str1"};
    testCase.wantResult = {"str1"};
    SUB_TEST("", testCase);

    testCase.strToAppend = {"str2"};
    testCase.wantResult = {"str1str2"};
    SUB_TEST("", testCase);

    testCase.strToAppend = {""};
    testCase.wantResult = {"str1str2"};
    SUB_TEST("14", testCase);

    testCase.strToAppend = {"a"};
    testCase.wantResult = {"str1str2a"};
    SUB_TEST("", testCase);

    testCase.strToAppend = {"aa"};
    testCase.wantError = true;
    SUB_TEST("", testCase);

    testCase.strToAppend = {""};
    testCase.wantResult = {"str1str2a"};
    testCase.wantError = false;
    SUB_TEST("", testCase);

    {
        AppendTestCase<0> testCase;

        testCase.strToAppend = {""};
        testCase.wantResult = {""};
        SUB_TEST("", testCase);

        testCase.strToAppend = {""};
        testCase.wantResult = {""};
        SUB_TEST("", testCase);

        testCase.strToAppend = {"1"};
        testCase.wantError = true;
        SUB_TEST("", testCase);
    }

    {
        AppendTestCase<4> testCase;

        testCase.strToAppend = {"abcde"};
        testCase.wantError = true;
        SUB_TEST("", testCase);
    }

    {
        AppendTestCase<30> testCase;

        testCase.appendType = INT64;
        testCase.int64ToAppend = 123;
        testCase.wantResult = {"123"};
        SUB_TEST("", testCase);

        testCase.appendType = STR;
        testCase.strToAppend = {"  "};
        testCase.wantResult = {"123  "};
        SUB_TEST("", testCase);

        testCase.appendType = INT64;
        testCase.int64ToAppend = -33;
        testCase.wantResult = {"123  -33"};
        SUB_TEST("", testCase);

        testCase.int64ToAppend = 1234567891011;
        testCase.wantResult = {"123  -331234567891011"};
        SUB_TEST("", testCase);

        testCase.appendType = STR;
        testCase.strToAppend = {"a"};
        testCase.wantResult = {"123  -331234567891011a"};
        SUB_TEST("", testCase);
    }


    {
        AppendTestCase<1> testCase;
        testCase.appendType = INT64;
        testCase.int64ToAppend = 1;
        testCase.wantResult = {"1"};
        SUB_TEST("", testCase);

        testCase.int64ToAppend = 2;
        testCase.wantError = true;
        SUB_TEST("", testCase);
    }
}

TEST(ArgcStringTest, ScanInt) {
    struct {
        string_view input, pattern, wantRest;
        int64 wantInt = 0;

        void test() const {
            string_view_c gotRest;
            int64 gotInt = scan_int64(string_view_c(input), string_view_c(pattern), gotRest);
            EXPECT_EQ(gotInt, wantInt);
            EXPECT_EQ(gotRest, wantRest);
        }
    } testCase;

    testCase = {
            .input = {"int=789"},
            .pattern = {"int="},
            .wantRest = {},
            .wantInt = 789,
    };
    SUB_TEST("", testCase);


    testCase = {
            .input = {"int=   789rest"},
            .pattern = {"int="},
            .wantRest = {"rest"},
            .wantInt = 789,
    };
    SUB_TEST("", testCase);


    testCase = {
            .input = {"int=      789rest"},
            .pattern = {"int=  "},
            .wantRest = {"rest"},
            .wantInt = 789,
    };
    SUB_TEST("", testCase);


    testCase = {
            .input = {"int=  \n  \n 789  rest"},
            .pattern = {"int=  "},
            .wantRest = {"  rest"},
            .wantInt = 789,
    };
    SUB_TEST("", testCase);


    testCase = {
            .input = {"   \n  int= 789rest"},
            .pattern = {" \nint=\n"},
            .wantRest = {},
            .wantInt = 0,
    };
    SUB_TEST("", testCase);


    testCase = {
            .input = {"   \n  int= 789rest"},
            .pattern = {"\n int=\n"},
            .wantRest = {},
            .wantInt = 0,
    };
    SUB_TEST("", testCase);


    testCase = {
            .input = {"   \n  int= 789\nrest"},
            .pattern = {" \n int=\n"},
            .wantRest = {"\nrest"},
            .wantInt = 789,
    };
    SUB_TEST("", testCase);


    testCase = {
            .input = {"   \n\n  int= 789\nrest"},
            .pattern = {"  \n  int=\n"},
            .wantRest = {"\nrest"},
            .wantInt = 789,
    };
    SUB_TEST("", testCase);


    testCase = {
            .input = {"   \n \n  int= 789\nrest"},
            .pattern = {"  \n  int=\n"},
            .wantRest = {},
            .wantInt = 0,
    };
    SUB_TEST("", testCase);


    testCase = {
            .input = {"   \n \n  int= 789\nrest"},
            .pattern = {"  \n\n  int=\n"},
            .wantRest = {},
            .wantInt = 0,
    };
    SUB_TEST("", testCase);


    testCase = {
            .input = {"   \n \n  int= 789\nrest"},
            .pattern = {"\n \n  int=\n"},
            .wantRest = {},
            .wantInt = 0,
    };
    SUB_TEST("", testCase);


    testCase = {
            .input = {"   \n  \n  int= 789\nrest"},
            .pattern = {" \n \n int=\n"},
            .wantRest = {"\nrest"},
            .wantInt = 789,
    };
    SUB_TEST("", testCase);


    testCase = {
            .input = {"int= 123456789101112\nrest"},
            .pattern = {" \n \n int=\n"},
            .wantRest = {"\nrest"},
            .wantInt = 123456789101112,
    };
    SUB_TEST("", testCase);


    testCase = {
            .input = {"     int =  -1234567891011rest\n"},
            .pattern = {" int =\n"},
            .wantRest = {"rest\n"},
            .wantInt = -1234567891011,
    };
    SUB_TEST("", testCase);


    testCase = {
            .input = {"     int =  -a789rest\n"},
            .pattern = {" int =\n"},
            .wantRest = {},
            .wantInt = 0,
    };
    SUB_TEST("", testCase);


    testCase = {
            .input = {"     int =  789rest\n"},
            .pattern = {" int \n"},
            .wantRest = {},
            .wantInt = 0,
    };
    SUB_TEST("", testCase);


    testCase = {
            .input = {"     int =  789rest\n"},
            .pattern = {"int:\n"},
            .wantRest = {},
            .wantInt = 0,
    };
    SUB_TEST("", testCase);

}

TEST(ArgcStringTest, ScanFloat) {
    struct {
        string_view input, pattern, wantRest;
        float64 wantFloat = 0;

        void test() const {
            string_view_c gotRest;
            float64 gotFloat = scan_float64(string_view_c(input), string_view_c(pattern), gotRest);
            EXPECT_EQ(gotFloat, wantFloat);
            EXPECT_EQ(gotRest, wantRest);
        }
    } testCase;


    testCase = {
            .input = {"f = 12.598"},
            .pattern = {"f="},
            .wantRest = {},
            .wantFloat = 0,
    };
    SUB_TEST("", testCase);


    testCase = {
            .input = {"f = 12.598"},
            .pattern = {" f ="},
            .wantRest = {""},
            .wantFloat = 12.598,
    };
    SUB_TEST("bad float", testCase);


    testCase = {
            .input = {"f = -0.598"},
            .pattern = {"f   ="},
            .wantRest = {""},
            .wantFloat = -0.598,
    };
    SUB_TEST("good", testCase);
}
