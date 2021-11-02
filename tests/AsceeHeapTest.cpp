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

#include <gtest/gtest.h>
#include <argc/types.h>

#define private public

#include "heap/Heap.h"

using namespace ascee;

class AsceeHeapTest : public ::testing::Test {
protected:
    Heap heap;
public:
    AsceeHeapTest() {
        heap.chunkIndex = {{full_id_t(1, 10), nullptr},
                           {full_id_t(1, 15), nullptr}};
    }
};

TEST_F(AsceeHeapTest, SimpleReadWrite) {
    auto modifier = heap.initSession(
            {{
                     .appID = 1,
                     .chunks = {{
                                        .id = 10,
                                        .maxNewSize = 12,
                                        .accessBlocks = {{4, 8, true}},
                                },
                                {
                                        .id = 15,
                                        .maxNewSize = 4,
                                        .accessBlocks = {},
                                },}
             },});

    std_id_t chunk = 10;
    modifier->saveVersion();
    modifier->loadContext(1);
    modifier->loadChunk(chunk);
    modifier->store<int64>(4, 123456);
    modifier->updateChunkSize(12);
    modifier->writeToHeap();

    modifier = heap.initSession(
            {{
                     .appID = 1,
                     .chunks = {{
                                        .id = 10,
                                        .maxNewSize = -1,
                                        .accessBlocks = {{4, 8, false}},
                                },}
             },});

    modifier->loadContext(1);
    chunk = 10;
    modifier->loadChunk(chunk);

    EXPECT_EQ(modifier->load<int64>(4), 123456);
    EXPECT_EQ(modifier->getChunkSize(), 12);
}

