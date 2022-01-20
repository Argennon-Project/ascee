// Copyright (c) 2022 aybehrouz <behrouz_ayati@yahoo.com>. All rights
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
#include <util/encoding.h>

using namespace argennon;
using namespace util;

TEST(UtilBase64utilTest, SimpleTest) {
    unsigned char binary[] = {0x11, 0x0, 0x4a, 0xff, 0x01};

    char buf[10];
    size_t len;

    base64urlEncode(binary, 5, buf, &len);

    EXPECT_STREQ(buf, "EQBK_wE");
    EXPECT_EQ(len, 7);

    EXPECT_EQ(base64urlEncode(binary, 5), "EQBK_wE");

    std::array<uint8_t, 10> dec{};

    EXPECT_THROW(base64urlDecode("abc-_", 5, dec.data()), std::invalid_argument);

    EXPECT_THROW(base64urlDecode("abc+", 4, dec.data()), std::invalid_argument);

    len = base64urlDecode("ab-_", 4, dec.data());

    std::array<uint8_t, 10> want = {0x69, 0xbf, 0xbf};
    EXPECT_EQ(dec, want);
    EXPECT_EQ(len, 3);

    EXPECT_EQ(base64DecodeLen(16), 12);
    EXPECT_EQ(base64DecodeLen(17), 0);
    EXPECT_EQ(base64DecodeLen(18), 13);
    EXPECT_EQ(base64DecodeLen(19), 14);
    EXPECT_EQ(base64DecodeLen(20), 15);
}
