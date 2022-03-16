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
#include "heap/RestrictedModifier.h"

using namespace argennon;
using namespace ascee;
using namespace runtime;
using std::vector;

using HeapModifier = RestrictedModifier;
using Access = AccessBlockInfo::Access::Type;
using SizeType = HeapModifier::ChunkInfo::ResizingType;

class HeapModifierDeathTest : public ::testing::Test {
protected:
    Chunk tempChunk1_10{256};
    Chunk tempChunk1_11{256};
    Chunk tempChunk1_100{256};
    Chunk tempChunk2_1{256};
    Chunk tempChunk2_2{256};


    std::unique_ptr<HeapModifier> modifier;

public:
    HeapModifierDeathTest() {
        tempChunk1_10.setSize(120);
        tempChunk1_11.setSize(256);
        tempChunk1_100.setSize(256);
        tempChunk2_1.setSize(256);
        tempChunk2_2.setSize(256);

        // std::vector<T>({obj1, obj2, obj3}) uses copy constructor of T. Therefore we can not use initializer
        // lists for classes without a copy constructor, and we need to use emplace instead.
        vector<HeapModifier::ChunkInfo> chunksForApp1;
        chunksForApp1.emplace_back(&tempChunk1_10, SizeType::read_only, 0,
                                   vector<int32>{100, 108, 150, 252},
                                   vector<AccessBlockInfo>{
                                           {8,  Access::read_only,    0},
                                           {16, Access::writable,     0},
                                           {4,  Access::int_additive, 0},
                                           {4,  Access::read_only,    0},
                                   });
        chunksForApp1.emplace_back(&tempChunk1_11, SizeType::read_only, 0,
                                   vector<int32>{100, 120},
                                   vector<AccessBlockInfo>{
                                           {8, Access::writable, 0},
                                           {8, Access::writable, 0},
                                   });
        chunksForApp1.emplace_back(&tempChunk1_100, SizeType::read_only, 0,
                                   vector<int32>{100},
                                   vector<AccessBlockInfo>{
                                           {8, Access::writable, 0},
                                   });

        vector<HeapModifier::ChunkInfo> chunksForApp2;
        chunksForApp2.emplace_back(&tempChunk2_1, SizeType::read_only, 0,
                                   vector<int32>{100},
                                   vector<AccessBlockInfo>{
                                           {8, Access::writable, 0},
                                   });
        chunksForApp2.emplace_back(&tempChunk2_2, SizeType::read_only, 0,
                                   vector<int32>{100},
                                   vector<AccessBlockInfo>{
                                           {8, Access::writable, 0},
                                   });

        vector<HeapModifier::ChunkMap64> appMaps;
        appMaps.emplace_back(vector<long_long_id>{{0, 10},
                                                  {0, 11},
                                                  {0, 100}}, std::move(chunksForApp1));
        appMaps.emplace_back(vector<long_long_id>{{0, 10},
                                                  {0, 11}}, std::move(chunksForApp2));

        modifier = std::make_unique<HeapModifier>(vector<long_id>{1, 2}, std::move(appMaps));

        *(int64*) tempChunk1_10.getContentPointer(100, 8).get() = 0x102020201020202;

        *(int64*) tempChunk1_11.getContentPointer(100, 8).get() = 789;
        *(int64*) tempChunk1_11.getContentPointer(120, 8).get() = 321;

    }
};

TEST_F(HeapModifierDeathTest, SimpleReadWrite) {
    modifier->saveVersion();
    modifier->loadContext(1);
    modifier->loadChunk(short_id(10));
    auto got = modifier->load<int64>(100);

    EXPECT_EQ(got, 0x102020201020202) << "got: 0x" << std::hex << got;

    auto got2 = modifier->load<util::StaticArray<int64, 1>>(100);
    EXPECT_EQ(got2[0], 0x102020201020202) << "got: 0x" << std::hex << got;

    EXPECT_THROW(modifier->load<int32>(150), std::out_of_range);

    EXPECT_TRUE(modifier->isValid(150, 4));

    modifier->addInt(150, 12);
    modifier->addInt(150, 22);

    EXPECT_THROW(modifier->addInt(150, int64(3)), std::out_of_range);

    modifier->addInt(150, 10);

    modifier->loadChunk(long_id(100));
    modifier->store<int64>(100, 123456789);
    got = modifier->load<int64>(100);
    EXPECT_EQ(got, 123456789);

    got = modifier->load<int16_t>(100, 2);
    EXPECT_EQ(got, 0x75b);

    modifier->store<byte>(100, 0x45, 7);
    got = modifier->load<int64>(100);
    EXPECT_EQ(got, 0x45000000075BCD15);

    modifier->loadChunk(short_id(10));
    EXPECT_THROW(modifier->store(100, 444444), std::out_of_range);

    int128 big = 1;
    big <<= 100;
    modifier->store(108, big);
    auto gotBig = modifier->load<int128>(108);
    EXPECT_TRUE(big == gotBig);

    modifier->loadContext(2);

    EXPECT_THROW(modifier->store(0, 0), std::out_of_range);

    modifier->loadChunk(short_id(10));
    EXPECT_THROW(modifier->store(100, big), std::out_of_range);

    EXPECT_THROW(modifier->load<int128>(100), std::out_of_range);

    modifier->store(100, 444555666777888);
    got = modifier->load<int64>(100);
    EXPECT_EQ(got, 444555666777888);

    modifier->loadContext(1);
    modifier->loadChunk(long_id(100));
    got = modifier->load<int64>(100);
    EXPECT_EQ(got, 0x45000000075BCD15);

    modifier->writeToHeap();

    EXPECT_EQ(*(int64*) tempChunk1_10.getContentPointer(150, 4).get(), 44);
}

TEST_F(HeapModifierDeathTest, VersionZero) {
    modifier->loadContext(1);
    modifier->loadChunk(short_id(10));
    auto got = modifier->load<int32>(252);
    EXPECT_EQ(got, 0);

    EXPECT_THROW(modifier->load<int64>(0), std::out_of_range);

    EXPECT_EXIT(modifier->restoreVersion(1), testing::KilledBySignal(SIGABRT), "");
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
    modifier->store<uint16>(120, 0x2233, 5);


    auto v3 = modifier->saveVersion();
    modifier->store<int64>(100, 3);
    modifier->store<int64>(120, 33);

    EXPECT_EQ(modifier->load<int64>(100), 3);
    EXPECT_EQ(modifier->load<int64>(120), 33);

    modifier->restoreVersion(v3);
    EXPECT_EQ(modifier->load<int64>(100), 2);
    EXPECT_EQ(modifier->load<int64>(120), 0x2233000000000b);

    modifier->restoreVersion(v2);
    EXPECT_EQ(modifier->load<int64>(100), 2);
    EXPECT_EQ(modifier->load<int64>(120), 0xb);

    EXPECT_EXIT(modifier->restoreVersion(v3), testing::KilledBySignal(SIGABRT), "");

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

TEST_F(HeapModifierDeathTest, ChunkExpansion) {
    Chunk tempChunk;
    tempChunk.reserveSpace(15);
    tempChunk.setSize(5);

    vector<HeapModifier::ChunkInfo> chunks;
    EXPECT_THROW(
            chunks.emplace_back(&tempChunk, SizeType::expandable, 15,
                                vector<int32>{12},
                                vector<AccessBlockInfo>{
                                        {4, Access::writable, 0},
                                }), std::out_of_range);

    chunks.emplace_back(&tempChunk, SizeType::expandable, 15,
                        vector<int32>{3, 11},
                        vector<AccessBlockInfo>{
                                {8, Access::writable, 0},
                                {4, Access::writable, 0},
                        });

    vector<HeapModifier::ChunkMap64> appMaps;
    appMaps.emplace_back(vector<long_long_id>{{123, 10}}, std::move(chunks));
    HeapModifier m({0x1000}, std::move(appMaps));

    // start of test:
    m.loadContext(0x1000);
    m.loadChunk(123, 10);
    m.saveVersion();

    EXPECT_EQ(m.getChunkSize(), 5);
    EXPECT_EQ(m.load<int64>(3), 0);
    m.store<int64>(3, 0x1122334455667788);
    m.store<int16>(11, 0x1020, 2);

    EXPECT_THROW(m.updateChunkSize(4), std::out_of_range);

    m.updateChunkSize(10);

    EXPECT_EQ(m.load<int32>(11), 0x10200000);

    auto v = m.saveVersion();
    m.store<int32>(11, 0x22222222);
    m.store<int32>(3, 0x22222222);

    EXPECT_EQ(m.load<int16>(11, 2), 0x2222);
    EXPECT_EQ(m.load<int32>(3), 0x22222222);
    m.updateChunkSize(15);

    EXPECT_EQ(m.getChunkSize(), 15);

    m.restoreVersion(v);

    EXPECT_EQ(m.getChunkSize(), 10);

    m.writeToHeap();

    tempChunk.setSize(15);
    EXPECT_EQ((std::string) tempChunk, "size: 15, capacity: 15, content: 0x[ 0 0 0 88 77 66 55 44 33 22 0 0 0 0 0 ]");
}
