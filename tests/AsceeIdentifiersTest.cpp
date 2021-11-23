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

#include "subtest.h"
#include <util/IdentifierTrie.h>

using namespace ascee;
using namespace runtime;

TEST(AsceeIdentifiersTest, SimpleTrie) {
    struct {
        byte input[16] = {};
        int maxLength = 4;
        uint64_t wantID = 0;
        int wantLen = 0;
        bool wantOutOfRange = false;
        IdentifierTrie<uint64_t, 4> t = IdentifierTrie<uint64_t, 4>({0x25, 0x25aa, 0x300000, 0xa1234560});

        void test() {
            uint64_t gotID;
            int gotLen;

            if (wantOutOfRange) {
                EXPECT_THROW(t.readIdentifier(input, &gotLen, maxLength), std::out_of_range);
                return;
            }

            gotID = t.readIdentifier(input, &gotLen, maxLength);

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


TEST(AsceeIdentifiersTest, VarUIntTest) {
    IdentifierTrie<uint64_t, 4> tr({0x45684515, 0x2012, 0x220000, 0x31015499});

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

TEST(AsceeIdentifiersTest, DifferentTries) {
    uint32_t id;
    int n;

    IdentifierTrie<uint32_t, 4> fixed({0, 0, 0, 0xffffffff});

    byte b2[] = {0x1, 0x2, 0x3, 0x4};
    id = fixed.readIdentifier(b2, &n);
    EXPECT_EQ(id, 0x01020304);
    EXPECT_EQ(n, 4);

    byte b3[] = {0xff, 0xff, 0xff, 0xfe};
    id = fixed.readIdentifier(b3, &n);
    EXPECT_EQ(id, 0xfffffffe);
    EXPECT_EQ(n, 4);

    EXPECT_THROW(fixed.readIdentifier(b3, &n, 3), std::out_of_range);

    byte b4[] = {0xff, 0xff, 0xff, 0xff};
    EXPECT_THROW(fixed.readIdentifier(b4, &n), std::out_of_range);

    EXPECT_THROW((IdentifierTrie<uint32_t, 3>({0x8, 0x8, 0xa0000})), std::invalid_argument);

    IdentifierTrie<uint32_t, 3> noLvl({0x8, 0x800, 0xa0000});

    byte b5[] = {0x8, 0x0, 0x0, 0x0};
    id = noLvl.readIdentifier(b5, &n);
    EXPECT_EQ(id, 0x08000000);
    EXPECT_EQ(n, 3);
}

TEST(AsceeIdentifiersTest, ParseSymbolic) {
    IdentifierTrie<uint8_t, 1> t({0x10});

    uint8_t id;

    t.parseIdentifier("5", id);
    EXPECT_EQ(id, 5);

    t.parseIdentifier("5.", id);
    EXPECT_EQ(id, 5);

    t.parseIdentifier(".5", id);
    EXPECT_EQ(id, 5);

    EXPECT_THROW(t.parseIdentifier("5. ", id), std::invalid_argument);

    EXPECT_THROW(t.parseIdentifier(".", id), std::out_of_range);

    EXPECT_THROW(t.parseIdentifier("", id), std::out_of_range);

    EXPECT_THROW(t.parseIdentifier("0x10", id), std::out_of_range);

    uint32_t id2;
    IdentifierTrie<uint32_t, 3> t2({0x10, 0x1111, 0x119900});

    t2.parseIdentifier("0x0f", id2);
    EXPECT_EQ(id2, 0x0f000000);

    EXPECT_THROW(t2.parseIdentifier("0x0f.0x0", id2), std::invalid_argument);

    EXPECT_THROW(t2.parseIdentifier("16.0.0  ", id2), std::invalid_argument);

    t2.parseIdentifier("16.0", id2);
    EXPECT_EQ(id2, 0x10000000);

    t2.parseIdentifier("16  .   17", id2);
    EXPECT_EQ(id2, 0x10110000);

    t2.parseIdentifier("0x11...0x2", id2);
    EXPECT_EQ(id2, 0x11020000);

    EXPECT_THROW(t2.parseIdentifier("420", id2), std::overflow_error);

    t2.parseIdentifier("0x11.0x11.0xaa", id2);
    EXPECT_EQ(id2, 0x1111aa00);

    EXPECT_THROW(t2.parseIdentifier("g2", id2), std::invalid_argument);
}
