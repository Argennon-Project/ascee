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
#include <heap/Chunk.h>

using namespace ascee;
using namespace runtime;

TEST(HeapChunkTest, ApplyDelta) {
    Digest good{}, bad{1};
    heap::Chunk c;
    using std::string;


    byte d1[] = {10, 0, 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    EXPECT_THROW(c.applyDelta(bad, d1, sizeof(d1)), std::invalid_argument);
    EXPECT_EQ("size: 0, capacity: 0, content: [ ]", (string) c);
    c.applyDelta(good, d1, sizeof(d1));
    EXPECT_EQ("size: 10, capacity: 10, content: [ 1 2 3 4 5 6 7 8 9 10 ]", (string) c);


    byte d2[] = {0, 0, 2, 1, 4, 7, 1, 5};

    EXPECT_THROW(c.applyDelta(bad, d2, sizeof(d2)), std::invalid_argument);
    EXPECT_EQ("size: 10, capacity: 10, content: [ 1 2 3 4 5 6 7 8 9 10 ]", (string) c);
    c.applyDelta(good, d2, sizeof(d2));
    EXPECT_EQ("size: 10, capacity: 10, content: [ 0 6 3 4 5 6 7 8 9 15 ]", (string) c);


    byte d3[] = {15, 3, 2, 4, 7};

    EXPECT_THROW(c.applyDelta(bad, d3, sizeof(d3)), std::invalid_argument);
    EXPECT_EQ("size: 10, capacity: 10, content: [ 0 6 3 4 5 6 7 8 9 15 ]", (string) c);
    c.applyDelta(good, d3, sizeof(d3));
    EXPECT_EQ("size: 5, capacity: 5, content: [ 0 6 3 0 2 ]", (string) c);


    byte d4[] = {10, 13, 1, 2, 0, 1, 7};

    EXPECT_THROW(c.applyDelta(bad, d4, sizeof(d4)), std::invalid_argument);
    EXPECT_EQ("size: 5, capacity: 5, content: [ 0 6 3 0 2 ]", (string) c);
    c.applyDelta(good, d4, sizeof(d4));
    EXPECT_EQ("size: 15, capacity: 15, content: [ 0 6 3 0 2 0 0 0 0 0 0 0 0 2 7 ]", (string) c);
}
