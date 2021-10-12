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
using namespace argcrt;

TEST(ArgcStringDeathTest, AppendStr) {
    STRING_BUFFER(strBuf, 10);

    append_str(&strBuf, STRING("str1"));
    EXPECT_STREQ(strBuf.buffer, "str1");

    append_str(&strBuf, STRING("str2"));
    EXPECT_STREQ(strBuf.buffer, "str1str2");

    append_str(&strBuf, STRING(""));
    EXPECT_STREQ(strBuf.buffer, "str1str2");

    append_str(&strBuf, STRING("a"));
    EXPECT_STREQ(strBuf.buffer, "str1str2a");

    EXPECT_EXIT(append_str(&strBuf, STRING("a")),
                testing::KilledBySignal(SIGSEGV),
                ""
    );

    STRING_BUFFER(strBuf1, 1);
    append_str(&strBuf1, STRING(""));
    append_str(&strBuf1, STRING(""));
    EXPECT_STREQ(strBuf1.buffer, "");

    EXPECT_EXIT(append_str(&strBuf1, STRING("1")),
                testing::KilledBySignal(SIGSEGV),
                ""
    );

    STRING_BUFFER(strBuf0, 0);
    EXPECT_EXIT(append_str(&strBuf0, STRING("")),
                testing::KilledBySignal(SIGSEGV),
                ""
    );

    STRING_BUFFER(strBuf2, 4);
    EXPECT_EXIT(append_str(&strBuf2, STRING("abcd")),
                testing::KilledBySignal(SIGSEGV),
                ""
    );
}

TEST(ArgcStringDeathTest, AppendInt) {
    STRING_BUFFER(strBuf, 30);

    append_int64(&strBuf, 123);
    EXPECT_STREQ(strBuf.buffer, "123");

    append_str(&strBuf, STRING("  "));
    append_int64(&strBuf, -33);
    EXPECT_STREQ(strBuf.buffer, "123  -33");

    append_int64(&strBuf, 1234567891011);
    append_str(&strBuf, STRING("a"));
    EXPECT_STREQ(strBuf.buffer, "123  -331234567891011a");

    STRING_BUFFER(strBuf2, 2);
    append_int64(&strBuf2, 1);
    EXPECT_STREQ(strBuf2.buffer, "1");
    EXPECT_EXIT(append_int64(&strBuf2, 1),
                testing::KilledBySignal(SIGSEGV),
                ""
    );
}
