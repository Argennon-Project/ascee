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

#include <gtest/gtest.h>
#include <argc/types.h>

#include <memory>

#include "heap/Chunk.h"
#include "heap/Modifier.h"

using namespace ascee;
using namespace runtime;
using std::vector;

class HeapModifierDeathTest : public ::testing::Test {
protected:
    heap::Chunk tempChunk1_10{256};
    heap::Chunk tempChunk1_11{256};
    heap::Chunk tempChunk1_100{256};
    heap::Chunk tempChunk2_1{256};
    heap::Chunk tempChunk2_2{256};


    std::unique_ptr<heap::Modifier> modifier;

public:
    HeapModifierDeathTest() {
        tempChunk1_10.setSize(120);
        tempChunk1_11.setSize(256);
        tempChunk1_100.setSize(256);
        tempChunk2_1.setSize(256);
        tempChunk2_2.setSize(256);

        // std::vector<T>({obj1, obj2, obj3}) uses copy constructor of T. Therefore we can not use initializer
        // lists for classes without a copy constructor, and we need to use emplace instead.
        vector<heap::Modifier::ChunkInfo> chunksForApp1;
        chunksForApp1.emplace_back(&tempChunk1_10, 0, false,
                                   vector<int32>{100, 108, 252},
                                   vector<BlockAccessInfo>{
                                           {8,  false, 0},
                                           {16, true,  0},
                                           {4,  false, 0},
                                   });
        chunksForApp1.emplace_back(&tempChunk1_11, 0, false,
                                   vector<int32>{100, 120},
                                   vector<BlockAccessInfo>{
                                           {8, true, 0},
                                           {8, true, 0},
                                   });
        chunksForApp1.emplace_back(&tempChunk1_100, 0, false,
                                   vector<int32>{100},
                                   vector<BlockAccessInfo>{
                                           {8, true, 0},
                                   });

        vector<heap::Modifier::ChunkInfo> chunksForApp2;
        chunksForApp2.emplace_back(&tempChunk2_1, 0, false,
                                   vector<int32>{100},
                                   vector<BlockAccessInfo>{
                                           {8, true, 0},
                                   });
        chunksForApp2.emplace_back(&tempChunk2_2, 0, false,
                                   vector<int32>{100},
                                   vector<BlockAccessInfo>{
                                           {8, true, 0},
                                   });

        vector<heap::Modifier::ChunkMap64> appMaps;
        appMaps.emplace_back(vector<long_id>{10, 11, 100}, std::move(chunksForApp1));
        appMaps.emplace_back(vector<long_id>{10, 11}, std::move(chunksForApp2));

        modifier = std::make_unique<heap::Modifier>(vector<long_id>{1, 2}, std::move(appMaps));

        *(int64*) tempChunk1_10.getContentPointer(100, 8).get(8) = 0x102020201020202;

        *(int64*) tempChunk1_11.getContentPointer(100, 8).get(8) = 789;
        *(int64*) tempChunk1_11.getContentPointer(120, 8).get(8) = 321;

    }
};

TEST_F(HeapModifierDeathTest, SimpleReadWrite) {
    modifier->saveVersion();
    modifier->loadContext(1);
    modifier->loadChunk(short_id(10));
    auto got = modifier->load<int64>(100);

    EXPECT_EQ(got, 0x102020201020202) << "got: 0x" << std::hex << got;

    auto got2 = modifier->load<StaticArray<int64, 1>>(100);
    EXPECT_EQ(got2.at(0), 0x102020201020202) << "got: 0x" << std::hex << got;

    modifier->loadChunk(long_id(100));
    modifier->store<int64>(100, 123456789);
    got = modifier->load<int64>(100);
    EXPECT_EQ(got, 123456789);

    modifier->loadChunk(short_id(10));
    EXPECT_THROW(modifier->store(100, 444444), std::out_of_range);

    int128 big = 1;
    big <<= 100;
    modifier->store(108, big);
    auto gotBig = modifier->load<int128>(108);
    EXPECT_TRUE(big == gotBig);

    modifier->loadContext(2);

    EXPECT_EXIT(modifier->store(0, 0), testing::KilledBySignal(SIGSEGV), "");

    modifier->loadChunk(short_id(10));
    EXPECT_THROW(modifier->store(100, big), std::out_of_range);

    EXPECT_THROW(modifier->load<int128>(100), std::out_of_range);

    modifier->store(100, 444555666777888);
    got = modifier->load<int64>(100);
    EXPECT_EQ(got, 444555666777888);

    modifier->loadContext(1);
    modifier->loadChunk(long_id(100));
    got = modifier->load<int64>(100);
    EXPECT_EQ(got, 123456789);
}

TEST_F(HeapModifierDeathTest, VersionZero) {
    modifier->loadContext(1);
    modifier->loadChunk(short_id(10));
    auto got = modifier->load<int32>(252);
    EXPECT_EQ(got, 0);

    EXPECT_THROW(modifier->load<int64>(0), std::out_of_range);

    EXPECT_THROW(modifier->restoreVersion(1), std::runtime_error);
}


TEST_F(HeapModifierDeathTest, SimpleVersioning) {
    modifier->loadContext(1);
    modifier->loadChunk(short_id(11));

    auto v0 = modifier->saveVersion();
    modifier->store<int64>(100, 1);
    modifier->store<int64>(120, 11);

    auto v1 = modifier->saveVersion();
    modifier->store<int64>(100, 2);

    auto v2 = modifier->saveVersion();
    modifier->store<int64>(120, 22);

    auto v3 = modifier->saveVersion();
    modifier->store<int64>(100, 3);
    modifier->store<int64>(120, 33);

    EXPECT_EQ(modifier->load<int64>(100), 3);
    EXPECT_EQ(modifier->load<int64>(120), 33);

    modifier->restoreVersion(v3);
    EXPECT_EQ(modifier->load<int64>(100), 2);
    EXPECT_EQ(modifier->load<int64>(120), 22);

    modifier->restoreVersion(v2);
    EXPECT_EQ(modifier->load<int64>(100), 2);
    EXPECT_EQ(modifier->load<int64>(120), 11);

    EXPECT_THROW(modifier->restoreVersion(v3), std::runtime_error);

    modifier->restoreVersion(v1);
    EXPECT_EQ(modifier->load<int64>(100), 1);
    EXPECT_EQ(modifier->load<int64>(120), 11);

    modifier->restoreVersion(v0);
    EXPECT_EQ(modifier->load<int64>(100), 789);
    EXPECT_EQ(modifier->load<int64>(120), 321);
}

TEST_F(HeapModifierDeathTest, RestoringMultiVersions) {
    modifier->loadContext(1);
    modifier->loadChunk(short_id(11));

    modifier->saveVersion();
    modifier->store<int64>(100, 1);
    modifier->store<int64>(120, 11);

    auto v1 = modifier->saveVersion();
    modifier->store<int64>(100, 2);

    modifier->saveVersion();
    modifier->store<int64>(120, 22);

    modifier->saveVersion();
    modifier->store<int64>(100, 3);
    modifier->store<int64>(120, 33);

    EXPECT_EQ(modifier->load<int64>(100), 3);
    EXPECT_EQ(modifier->load<int64>(120), 33);

    modifier->restoreVersion(v1);
    EXPECT_EQ(modifier->load<int64>(100), 1);
    EXPECT_EQ(modifier->load<int64>(120), 11);

    modifier->store<int64>(120, 44);
    auto v4 = modifier->saveVersion();

    modifier->store<int64>(100, 5);
    modifier->store<int64>(120, 55);

    modifier->saveVersion();
    modifier->store<int64>(100, 6);

    modifier->restoreVersion(v4);
    EXPECT_EQ(modifier->load<int64>(100), 1);
    EXPECT_EQ(modifier->load<int64>(120), 44);

    modifier->restoreVersion(0);
    EXPECT_EQ(modifier->load<int64>(100), 789);
    EXPECT_EQ(modifier->load<int64>(120), 321);
}
