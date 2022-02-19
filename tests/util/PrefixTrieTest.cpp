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
#include "arg/primitives.h"
#include "util/PrefixTrie.hpp"
#include "util/StaticArray.hpp"
#include "util/OrderedStaticMap.hpp"

using namespace argennon;
using namespace util;

TEST(PrefixTrieTest, SimpleTrie) {
    struct {
        byte input[16] = {};
        int maxLength = 4;
        uint64_t wantID = 0;
        int wantLen = 0;
        bool wantOutOfRange = false;
        PrefixTrie<uint64_t, 4> t = PrefixTrie<uint64_t, 4>({0x25, 0x25aa, 0x300000, 0xa1234560});

        void test() {
            uint64_t gotID;
            int gotLen;

            if (wantOutOfRange) {
                EXPECT_THROW(t.readPrefixCode(input, &gotLen, maxLength), std::out_of_range);
                return;
            }

            gotID = t.readPrefixCode(input, &gotLen, maxLength);

            EXPECT_EQ(gotID, wantID);
            EXPECT_EQ(gotLen, wantLen);
        }

    } testCase;


    testCase = {
            .input = {0x0, 0xff},
            .wantID = 0,
            .wantLen = 1
    };
    SUB_TEST("", testCase);


    testCase = {
            .input = {0x24, 0xff},
            .wantID = 0x2400000000000000,
            .wantLen = 1
    };
    SUB_TEST("", testCase);


    testCase = {
            .input = {0x25, 0x0},
            .wantID = 0x2500000000000000,
            .wantLen = 2
    };
    SUB_TEST("", testCase);


    testCase = {
            .input =  {0x25, 0x12},
            .wantID = 0x2512000000000000,
            .wantLen = 2
    };
    SUB_TEST("", testCase);


    testCase = {
            .input =  {0x25, 0xa9, 0x1},
            .wantID = 0x25a9000000000000,
            .wantLen = 2
    };
    SUB_TEST("", testCase);


    testCase = {
            .input =  {0x25, 0xaa, 0x0, 0x1},
            .wantID = 0x25aa000000000000,
            .wantLen = 3
    };
    SUB_TEST("", testCase);


    testCase = {
            .input =  {0x25, 0xaa, 0x1, 0x1},
            .wantID = 0x25aa010000000000,
            .wantLen = 3
    };
    SUB_TEST("", testCase);


    testCase = {
            .input =  {0x30, 0x0, 0x0, 0x0, 0x1},
            .wantID = 0x3000000000000000,
            .wantLen = 4
    };
    SUB_TEST("", testCase);


    testCase = {
            .input =  {0x30, 0x1, 0x1, 0x1},
            .wantID = 0x3001010100000000,
            .wantLen = 4
    };
    SUB_TEST("", testCase);


    testCase = {
            .input = {0xa1, 0x23, 0x45, 0x60, 0x0},
            .wantOutOfRange = true
    };
    SUB_TEST("", testCase);

    // maxLength test:
    testCase = {
            .input = {0x25, 0xaa, 0x1, 0x1},
            .maxLength = 5,
            .wantID = 0x25aa010000000000,
            .wantLen = 3,
    };
    SUB_TEST("", testCase);

    testCase = {
            .input = {0x25, 0xaa, 0x1, 0x1},
            .maxLength = 3,
            .wantID = 0x25aa010000000000,
            .wantLen = 3,
    };
    SUB_TEST("", testCase);

    testCase = {
            .input = {0x25, 0xaa, 0x1, 0x1},
            .maxLength = 2,
            .wantOutOfRange = true
    };
    SUB_TEST("", testCase);

    testCase = {
            .input =  {0x0},
            .maxLength = 1,
            .wantID = 0x0,
            .wantLen = 1,
    };
    SUB_TEST("", testCase);

    testCase = {
            .input =  {0x0},
            .maxLength = 0,
            .wantOutOfRange = true
    };
    SUB_TEST("", testCase);
}


TEST(PrefixTrieTest, VarUIntTest) {
    PrefixTrie<uint32_t, 4> varSize({0xd0, 0xf000, 0xfc0000, 0xffffff00});

    auto x = varSize.encodeVarUInt(8 * 1024);
    printf("x=%x\n", x);

    PrefixTrie<uint64_t, 4> tr({0x45684515, 0x2012, 0x220000, 0x31015499});
    StaticArray<byte, 16> buf = {};
    byte* ptr = &buf + 8;

    int n;
    auto code = tr.encodeVarUInt(11, &n);
    *(uint64_t*) ptr = code;
    ptr -= n;
    int m;
    uint64_t v = tr.decodeVarUInt(code, &m);
    printf("%lx: %d -> %ld m:%d\n", code, n, v, m);

    code = tr.encodeVarUInt(21, &n);
    *(uint64_t*) ptr = code;
    ptr -= n;
    v = tr.decodeVarUInt(code, &m);
    printf("%lx: %d -> %ld m:%d\n", code, n, v, m);

    code = tr.encodeVarUInt(77, &n);
    *(uint64_t*) ptr = code;
    ptr -= n;
    v = tr.decodeVarUInt(code, &m);
    printf("%lx: %d -> %ld m:%d\n", code, n, v, m);

    code = tr.encodeVarUInt(24457169, &n);
    StaticArray<byte, 10> temp = {};
    tr.writeBigEndian(&temp, code, n);
    printf("buf2: %s n:%d code:%lx\n", temp.toString().c_str(), n, code);

    *(uint64_t*) ptr = code;
    ptr -= n;
    v = tr.decodeVarUInt(code, &m);
    printf("%lx: %d -> %ld m:%d\n", code, n, v, m);

    ptr = &buf + 8;
    v = tr.decodeVarUInt(*(uint64_t*) ptr, &n);
    printf("%ld\n", v);
    ptr -= n;

    v = tr.decodeVarUInt(*(uint64_t*) ptr, &n);
    printf("%ld\n", v);
    ptr -= n;

    v = tr.decodeVarUInt(*(uint64_t*) ptr, &n);
    printf("%ld\n", v);
    ptr -= n;

    v = tr.decodeVarUInt(*(uint64_t*) ptr, &n);
    printf("%ld\n", v);
    ptr -= n;

    printf("%s\n", buf.toString().c_str());
}

TEST(PrefixTrieTest, DifferentTries) {
    uint32_t id;
    int n;

    PrefixTrie<uint32_t, 4> fixed({0, 0, 0, 0xffffffff});

    byte b2[] = {0x1, 0x2, 0x3, 0x4};
    id = fixed.readPrefixCode(b2, &n);
    EXPECT_EQ(id, 0x01020304);
    EXPECT_EQ(n, 4);

    byte b3[] = {0xff, 0xff, 0xff, 0xfe};
    id = fixed.readPrefixCode(b3, &n);
    EXPECT_EQ(id, 0xfffffffe);
    EXPECT_EQ(n, 4);

    EXPECT_THROW(fixed.readPrefixCode(b3, &n, 3), std::out_of_range);

    byte b4[] = {0xff, 0xff, 0xff, 0xff};
    EXPECT_THROW(fixed.readPrefixCode(b4, &n), std::out_of_range);

    EXPECT_THROW((PrefixTrie<uint32_t, 3>({0x8, 0x8, 0xa0000})), std::invalid_argument);

    PrefixTrie<uint32_t, 3> noLvl({0x8, 0x800, 0xa0000});

    byte b5[] = {0x8, 0x0, 0x0, 0x0};
    id = noLvl.readPrefixCode(b5, &n);
    EXPECT_EQ(id, 0x08000000);
    EXPECT_EQ(n, 3);
}

TEST(PrefixTrieTest, ParseSymbolic) {
    uint8_t id;

    PrefixTrie<uint32_t, 4> varSize({0xd0, 0xf000, 0xfc0000, 0xffffff00});

    EXPECT_EQ(varSize.uncheckedParse("0xab0"), 0x0ab00000);
    EXPECT_EQ(varSize.uncheckedParse("0x0ab0"), 0x0ab00000);
    EXPECT_EQ(varSize.uncheckedParse("0xab00"), 0xab000000);
    EXPECT_EQ(varSize.uncheckedParse("0x00ab00"), 0xab000000);
    EXPECT_EQ(varSize.uncheckedParse("0xabcde"), 0x0abcde00);
    EXPECT_EQ(varSize.uncheckedParse("0x00abcdef"), 0xabcdef00);
    EXPECT_EQ(varSize.uncheckedParse("0xabcdef0"), 0x0abcdef0);
    EXPECT_EQ(varSize.uncheckedParse("0xabcdef00"), 0xabcdef00);
    EXPECT_EQ(PrefixTrie<uint64_t>::uncheckedParse("0x123"), 0x123000000000000);
    EXPECT_EQ(PrefixTrie<uint64_t>::uncheckedParse("0x0abcde"), 0xabcde0000000000);


    //EXPECT_THROW(varSize.uncheckedParse("0x0ab00"), std::runtime_error);

    PrefixTrie<uint8_t, 1> t({0x10});


    t.parsePrefixCode("5", id);
    EXPECT_EQ(id, 5);

    t.parsePrefixCode("5.", id);
    EXPECT_EQ(id, 5);

    t.parsePrefixCode(".5", id);
    EXPECT_EQ(id, 5);

    EXPECT_THROW(t.parsePrefixCode("5. ", id), std::invalid_argument);

    EXPECT_THROW(t.parsePrefixCode(".", id), std::out_of_range);

    EXPECT_THROW(t.parsePrefixCode("", id), std::out_of_range);

    EXPECT_THROW(t.parsePrefixCode("0x10", id), std::out_of_range);

    uint32_t id2;
    PrefixTrie<uint32_t, 3> t2({0x10, 0x1111, 0x119900});

    t2.parsePrefixCode("0x0f", id2);
    EXPECT_EQ(id2, 0x0f000000);

    EXPECT_THROW(t2.parsePrefixCode("0x0f.0x0", id2), std::invalid_argument);

    EXPECT_THROW(t2.parsePrefixCode("16.0.0  ", id2), std::invalid_argument);

    t2.parsePrefixCode("16.0", id2);
    EXPECT_EQ(id2, 0x10000000);

    t2.parsePrefixCode("16  .   17", id2);
    EXPECT_EQ(id2, 0x10110000);

    t2.parsePrefixCode("0x11...0x2", id2);
    EXPECT_EQ(id2, 0x11020000);

    EXPECT_THROW(t2.parsePrefixCode("420", id2), std::overflow_error);

    t2.parsePrefixCode("0x11.0x11.0xaa", id2);
    EXPECT_EQ(id2, 0x1111aa00);

    EXPECT_THROW(t2.parsePrefixCode("g2", id2), std::invalid_argument);
}

TEST(PrefixTrieTest, ToDecimal) {
    PrefixTrie<uint64_t, 4> t({0x40, 0x7050, 0x800010, 0xbb001122});

    EXPECT_EQ(t.toDecimalStr(0x212350000000000), "2");

    EXPECT_EQ(t.toDecimalStr(0x4511125450000000), "69.17");

    EXPECT_EQ(t.toDecimalStr(0x7766201000000000), "119.102.32");

    EXPECT_EQ(t.toDecimalStr(0xbb00000000000000), "187.0.0.0");

    EXPECT_THROW(t.toDecimalStr(0xf000000000000000), std::out_of_range);
}
