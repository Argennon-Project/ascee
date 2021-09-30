#include "gtest/gtest.h"
#include "../include/argc/types.h"
#include "../include/argc/functions.h"


TEST(StringTest, Append) {
    StringBuffer(sbuf, 10);
    append_int64(&sbuf, 12345);
    EXPECT_STREQ(sbuf.buffer, "12345");
}
