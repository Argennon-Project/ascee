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

#include <heap/IdentifierTrie.h>
#include "gtest/gtest.h"

using namespace ascee;

TEST(AsceeIdentifiersTest, SimpleTrie) {
    IdentifierTrie<uint64_t, 4> t({0x25, 0x25aa, 0x300000, 0xa1234560});

    uint64_t id;
    int n;

    byte b1[] = {0x0, 0xff};
    n = t.readIdentifier(b1, id);
    EXPECT_EQ(id, 0);
    EXPECT_EQ(n, 1);

    byte b2[] = {0x24, 0xff};
    n = t.readIdentifier(b2, id);
    EXPECT_EQ(id, 0x2400000000000000);
    EXPECT_EQ(n, 1);

    byte b3[] = {0x25, 0x0};
    n = t.readIdentifier(b3, id);
    EXPECT_EQ(id, 0x2500000000000000);
    EXPECT_EQ(n, 2);

    byte b4[] = {0x25, 0x12};
    n = t.readIdentifier(b4, id);
    EXPECT_EQ(id, 0x2512000000000000);
    EXPECT_EQ(n, 2);

    byte b5[] = {0x25, 0xa9, 0x1};
    n = t.readIdentifier(b5, id);
    EXPECT_EQ(id, 0x25a9000000000000);
    EXPECT_EQ(n, 2);

    byte b6[] = {0x25, 0xaa, 0x0, 0x1};
    n = t.readIdentifier(b6, id);
    EXPECT_EQ(id, 0x25aa000000000000);
    EXPECT_EQ(n, 3);

    byte b7[] = {0x25, 0xaa, 0x1, 0x1};
    n = t.readIdentifier(b7, id);
    EXPECT_EQ(id, 0x25aa010000000000);
    EXPECT_EQ(n, 3);

    byte b8[] = {0x30, 0x0, 0x0, 0x0, 0x1};
    n = t.readIdentifier(b8, id);
    EXPECT_EQ(id, 0x3000000000000000);
    EXPECT_EQ(n, 4);

    byte b9[] = {0x30, 0x1, 0x1, 0x1};
    n = t.readIdentifier(b9, id);
    EXPECT_EQ(id, 0x3001010100000000);
    EXPECT_EQ(n, 4);

    byte b10[] = {0xa1, 0x23, 0x45, 0x60, 0x0};
    EXPECT_THROW(t.readIdentifier(b10, id, 5), std::out_of_range);

    // maxLength test:
    byte a1[] = {0x25, 0xaa, 0x1, 0x1};;
    n = t.readIdentifier(a1, id, 5);
    EXPECT_EQ(id, 0x25aa010000000000);
    EXPECT_EQ(n, 3);

    n = t.readIdentifier(a1, id, 3);
    EXPECT_EQ(id, 0x25aa010000000000);
    EXPECT_EQ(n, 3);

    EXPECT_THROW(t.readIdentifier(a1, id, 2), std::out_of_range);

    byte a2[1] = {0x0};;
    n = t.readIdentifier(a2, id, 1);
    EXPECT_EQ(id, 0x0);
    EXPECT_EQ(n, 1);

    EXPECT_THROW(t.readIdentifier(a2, id, 0), std::out_of_range);
}

TEST(AsceeIdentifiersTest, DifferentTries) {
    IdentifierTrie<uint32_t, 0> t({});
    uint32_t id, n;
    byte b1[] = {0x0};;
    EXPECT_THROW(t.readIdentifier(b1, id, 1), std::out_of_range);

    IdentifierTrie<uint32_t, 4> fixed({0, 0, 0, 0xffffffff});

    byte b2[] = {0x1, 0x2, 0x3, 0x4};
    n = fixed.readIdentifier(b2, id);
    EXPECT_EQ(id, 0x01020304);
    EXPECT_EQ(n, 4);

    byte b3[] = {0xff, 0xff, 0xff, 0xfe};
    n = fixed.readIdentifier(b3, id);
    EXPECT_EQ(id, 0xfffffffe);
    EXPECT_EQ(n, 4);

    EXPECT_THROW(fixed.readIdentifier(b3, id, 3), std::out_of_range);

    byte b4[] = {0xff, 0xff, 0xff, 0xff};
    EXPECT_THROW(fixed.readIdentifier(b4, id), std::out_of_range);

    EXPECT_THROW((IdentifierTrie<uint32_t, 3>({0x8, 0x8, 0xa0000})), std::invalid_argument);

    IdentifierTrie<uint32_t, 3> noLvl({0x8, 0x800, 0xa0000});

    byte b5[] = {0x8, 0x0, 0x0, 0x0};
    n = noLvl.readIdentifier(b5, id);
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
