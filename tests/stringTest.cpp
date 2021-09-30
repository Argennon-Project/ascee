#include "gtest/gtest.h"
#include "../include/argc/types.h"
#include "../include/argc/functions.h"


TEST(StringTest, AppendStr) {
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

TEST(StringTest, AppendInt) {
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
