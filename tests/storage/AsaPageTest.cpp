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

#include <string>
#include "subtest.h"
#include "storage/Page.h"

using namespace argennon;
using namespace asa;
using std::string;

TEST(AsaPageTest, ApplyDelta) {
    Page page(789);

    Page::Delta delta = {
            {1, 0xa, 0xb, 0xc, 1, 7, 7, 0, 0, // identifiers
                    // chunks:
                    8, 1, 2, 3, 3, 0, // native
                    0, 0, // migrant 0xabc
                    3, 2, 3, 14, 15}, // migrant 0x770
            {}
    };

    VarLenFullID id(static_cast<std::unique_ptr<byte[]>>(new byte[3]{0x20, 0x15, 0x2}));
    page.applyDelta(id, delta, 800);

    EXPECT_EQ((string) *page.getNative(), "size: 8, capacity: 8, content: 0x[ 3 3 0 0 0 0 0 0 ]");

    EXPECT_EQ((string) page.getMigrants()[0].id, "0xabc");
    EXPECT_EQ((string) *page.getMigrants()[0].chunk, "size: 0, capacity: 0, content: 0x[ ]");

    EXPECT_EQ((string) page.getMigrants()[1].id, "0x770");
    EXPECT_EQ((string) *page.getMigrants()[1].chunk, "size: 3, capacity: 3, content: 0x[ 0 e f ]");
}
