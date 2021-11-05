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

TEST(AsceeIdentifierTest, SimpleTrie) {
    IdentifierTrie<uint64_t, 4> t({0x25, 0x25aa, 0x300000, 0xa1234560});

    uint64_t id;
    int n;

    byte b1[] = {0x0, 0xff};
    n = t.readIdentifier(id, b1);
    EXPECT_EQ(id, 0);
    EXPECT_EQ(n, 1);

    byte b2[] = {0x24, 0xff};
    n = t.readIdentifier(id, b2);
    EXPECT_EQ(id, 0x2400000000000000);
    EXPECT_EQ(n, 1);

    byte b3[] = {0x25, 0x0};
    n = t.readIdentifier(id, b3);
    EXPECT_EQ(id, 0x2500000000000000);
    EXPECT_EQ(n, 2);

    byte b4[] = {0x25, 0x12};
    n = t.readIdentifier(id, b4);
    EXPECT_EQ(id, 0x2512000000000000);
    EXPECT_EQ(n, 2);

    byte b5[] = {0x25, 0xa9, 0x1};
    n = t.readIdentifier(id, b5);
    EXPECT_EQ(id, 0x25a9000000000000);
    EXPECT_EQ(n, 2);

    byte b6[] = {0x25, 0xaa, 0x0, 0x1};
    n = t.readIdentifier(id, b6);
    EXPECT_EQ(id, 0x25aa000000000000);
    EXPECT_EQ(n, 3);

    byte b7[] = {0x25, 0xaa, 0x1, 0x1};
    n = t.readIdentifier(id, b7);
    EXPECT_EQ(id, 0x25aa010000000000);
    EXPECT_EQ(n, 3);

    byte b8[] = {0x30, 0x0, 0x0, 0x0, 0x1};
    n = t.readIdentifier(id, b8);
    EXPECT_EQ(id, 0x3000000000000000);
    EXPECT_EQ(n, 4);

    byte b9[] = {0x30, 0x1, 0x1, 0x1};
    n = t.readIdentifier(id, b9);
    EXPECT_EQ(id, 0x3001010100000000);
    EXPECT_EQ(n, 4);

    byte b10[] = {0xa1, 0x23, 0x45, 0x60, 0x0};
    EXPECT_THROW(t.readIdentifier(id, b10, 5), std::out_of_range);
}
