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

#include "heap/PageCache.h"

using namespace ascee;
using namespace runtime;

class AsceeHeapDeathTest : public ::testing::Test {
protected:
    Heap heap;
public:
    AsceeHeapDeathTest() {
        heap = Heap();

        heap.chunkIndex = {
                {full_id(1, 10), nullptr},
                {full_id(1, 12), nullptr},
                {full_id(1, 15), nullptr},
                {full_id(2, 10), nullptr},
                {full_id(2, 11), nullptr},
                {full_id(2, 15), nullptr},
        };
        auto modifier = heap.initSession(
                {{
                         .appID = 1,
                         .chunks = {
                                 {10, 16, {
                                                  {0, 8, true},
                                                  {8, 8, true},
                                          }},
                                 {12, 8,  {
                                                  {0, 8, true},
                                          }},
                                 {15, 4,  {
                                                  {0, 4, true},
                                          }},
                         }
                 },});
        modifier.saveVersion();
        modifier.loadContext(1);
        modifier.loadChunk(short_id(10));
        modifier.store<int64>(0, 12345678910);
        modifier.store<int64>(8, 77777777777);
        modifier.updateChunkSize(16);

        modifier.loadChunk(short_id(12));
        modifier.store<int64>(0, 0x1011121314151617);
        modifier.updateChunkSize(8);

        modifier.loadChunk(short_id(15));
        modifier.store<int32>(0, 0x33333333);
        modifier.updateChunkSize(4);

        modifier.writeToHeap();
    }
};

TEST_F(AsceeHeapDeathTest, SimpleRead) {
    auto modifier = heap.initSession(
            {{
                     .appID = 1,
                     .chunks = {
                             {10, -1, {{8, 8, false}}},
                             {15, -1, {{2, 1, false}}},
                     }
             },});

    modifier.loadContext(1);

    modifier.loadChunk(short_id(10));
    EXPECT_EQ(modifier.load<int64>(8), 77777777777);

    modifier.loadChunk(short_id(15));
    EXPECT_EQ(modifier.load<byte>(2), 0x33);

    EXPECT_THROW(
            heap.initSession({{.appID = 1, .chunks = {{11, -1, {{8, 8, false}}}}},}),
            std::runtime_error
    );

    EXPECT_THROW(
            heap.initSession({{.appID = 1, .chunks = {{11, 0, {{8, 8, false}}}}},}),
            std::runtime_error
    );

    EXPECT_THROW(
            heap.initSession({{.appID = 1, .chunks = {{11, 10, {{8, 8, false}}}}},}),
            std::runtime_error
    );

    EXPECT_THROW(
            heap.initSession({{.appID = 2, .chunks = {{10, -1, {{8, 8, false}}}}},}),
            std::out_of_range
    );

    EXPECT_THROW(
            heap.initSession({{.appID = 1, .chunks = {{10, -1, {{8, 9, false}}}}},}),
            std::out_of_range
    );

    EXPECT_THROW(
            heap.initSession({{.appID = 1, .chunks = {{10, -1, {{17, 8, false}}}}},}),
            std::out_of_range
    );
}

TEST_F(AsceeHeapDeathTest, ChunkCreation) {
    EXPECT_THROW(
            heap.initSession({{2, {{10, 10, {{1, 10, false}}},}},}),
            std::out_of_range
    );

    EXPECT_THROW(
            heap.initSession({{2, {{10, 2 * 1024 * 1024, {{1, 10, false}}},}},}),
            std::out_of_range
    );

    EXPECT_THROW(
            heap.initSession({{2, {{10, -1, {{1, 10, false}}},}},}),
            std::out_of_range
    );

    EXPECT_THROW(
            heap.initSession({{2, {{10, 0, {{8 * 1024, 1, false}}},}},}),
            std::out_of_range // chunk not found
    );

    EXPECT_THROW(
            heap.initSession({{2, {{10, 10, {{0, 1, false}}}, {10, 12, {{0, 12, false}}}}},}),
            std::invalid_argument // duplicate chunk
    );

    EXPECT_THROW(
            heap.initSession({
                                     {2, {{10, 12, {{1, 10, false}}}}},
                                     {1, {{10, -1, {{0, 1,  false}}}, {10, 12, {{0, 1, false}}}}},}),
            std::invalid_argument // chunk already exist
    );

    EXPECT_THROW(
            heap.initSession({
                                     {2, {{10, 12, {{1, 10, false}}}}},
                                     {1, {{10, -1, {{0, 1,  false}, {0, 2, true}}}}}}),
            std::invalid_argument // block already exist
    );

    EXPECT_THROW(
            heap.initSession({{1, {{10, -1, {{0, 1, false}}}, {10, -1, {{1, 1, false}}}}},}),
            std::invalid_argument // chunk already exist
    );

    auto modifier = heap.initSession(
            {{
                     .appID = 2,
                     .chunks = {
                             {10, 10,  {{0,   8, true}}},
                             {11, 18,  {{10,  8, true}}},
                             {15, 129, {{128, 1, true}}},
                     }},
            });

    modifier.loadContext(2);
    modifier.saveVersion();

    modifier.loadChunk(long_id(10));
    EXPECT_EQ(modifier.load<int64>(0), 0);
    modifier.store(0, 0x1122334455667788);

    modifier.loadChunk(long_id(11));
    modifier.store(10, 0x1111111111111111);

    modifier.loadChunk(long_id(15));
    EXPECT_EQ(modifier.load<byte>(128), 0);
    modifier.store<byte>(128, 0x22);

    modifier.loadChunk(long_id(11));
    EXPECT_EQ(modifier.load<int64>(10), 0x1111111111111111);

    modifier.loadChunk(long_id(15));
    EXPECT_EQ(modifier.load<byte>(128), 0x22);
    modifier.updateChunkSize(0);

    modifier.loadChunk(long_id(10));
    modifier.updateChunkSize(10);

    modifier.writeToHeap();

    EXPECT_THROW(
            heap.initSession({{2, {{11, -1, {{10, 1, false}}},}},}),
            std::out_of_range
    );

    EXPECT_THROW(
            heap.initSession({{2, {{15, -1, {{128, 1, false}}},}},}),
            std::out_of_range
    );

    EXPECT_THROW(
            heap.initSession({{2, {{10, -1, {{7, 4, false}}},}},}),
            std::out_of_range
    );

    {
        auto modifier = heap.initSession({{2, {{10, -1, {{6, 4, false}}},}},});

        modifier.loadContext(2);
        modifier.loadChunk(long_id(10));
        EXPECT_EQ(modifier.load<int32>(6), 0x1122);
    }
}

TEST_F(AsceeHeapDeathTest, ChunkRemoval) {
    auto modifier = heap.initSession(
            {{
                     .appID = 1,
                     .chunks = {
                             {10, 0, {}},
                             {12, 0, {}},
                             {15, 5, {{2, 1, true}}},
                     }
             },});

    modifier.saveVersion();
    modifier.loadContext(1);
    modifier.loadChunk(short_id(10));
    modifier.updateChunkSize(0);

    modifier.loadChunk(short_id(15));
    modifier.updateChunkSize(0);

    EXPECT_EQ(modifier.load<byte>(2), 0x33);
    modifier.store<byte>(2, 0x88);
    EXPECT_EQ(modifier.load<byte>(2), 0x88);

    modifier.writeToHeap();

    EXPECT_THROW(
            heap.initSession({{1, {{10, -1, {}}}},}),
            std::out_of_range
    );

    EXPECT_THROW(
            heap.initSession({{1, {{15, -1, {}}}},}),
            std::out_of_range
    );

    {
        auto modifier = heap.initSession({{1, {
                {15, 10, {{2, 1, true}}},
                {12, -1, {{0, 8, false}}},
        }},});

        modifier.loadContext(1);
        modifier.loadChunk(short_id(15));
        EXPECT_EQ(modifier.load<byte>(2), 0);
    }
}

TEST_F(AsceeHeapDeathTest, ChunkResizing) {
    EXPECT_THROW(
            heap.initSession({{.appID = 1, .chunks = {{10, 8, {{16, 1, false}}},}},}),
            std::out_of_range
    );

    auto modifier = heap.initSession(
            {{
                     .appID = 1,
                     .chunks = {
                             {10, 8,  {{12, 4, true}}},
                             {15, 16, {{8,  8, true}}},
                     }},
            });

    modifier.saveVersion();
    modifier.loadContext(1);

    modifier.loadChunk(long_id(10));
    EXPECT_EQ(modifier.load<int32>(12), 18);
    modifier.store<int32>(12, 55555);
    EXPECT_EQ(modifier.load<int32>(12), 55555);

    modifier.updateChunkSize(6);

    modifier.loadChunk(long_id(15));
    EXPECT_EQ(modifier.load<int64>(8), 0);
    modifier.store<int64>(8, 1515151515);

    EXPECT_THROW(modifier.updateChunkSize(17), std::out_of_range);
    modifier.updateChunkSize(16);

    modifier.writeToHeap();

    EXPECT_THROW(
            heap.initSession({{.appID = 1, .chunks = {{10, -1, {{6, 1, false}}},}},}),
            std::out_of_range
    );

    EXPECT_THROW(
            heap.initSession({{.appID = 1, .chunks = {{15, -1, {{14, 3, false}}},}},}),
            std::out_of_range
    );

    {
        auto modifier = heap.initSession(
                {{
                         .appID = 1,
                         .chunks = {
                                 {15, -1, {
                                         {0, 4, false},
                                         {8, 8, true}
                                 }},
                         }},
                });

        modifier.saveVersion();
        modifier.loadContext(1);
        modifier.loadChunk(long_id(15));
        EXPECT_EQ(modifier.load<int64>(8), 1515151515);
        EXPECT_EQ(modifier.load<int32>(0), 0x33333333);
    }
}

