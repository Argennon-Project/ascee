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

#include <gmock/gmock.h>
#include "subtest.h"
#include "storage/PageLoader.h"
#include "validator/RequestScheduler.h"
#include "storage/PageCache.h"

using namespace argennon;
using namespace ave;
using namespace asa;
using ::testing::Return, std::vector;


using Access = AccessBlockInfo::Access::Type;
using ClusterType = std::vector<AppRequestIdType>;
constexpr long_long_id chunk1_local_id(0x4400000000000000, 0x0500000000000000);
constexpr long_id app_1_id(0x1000000000000000);

class RequestSchedulerTest : public ::testing::Test {
protected:
    PageLoader pl{};
    PageCache pc;
    ChunkIndex singleChunk;

public:
    RequestSchedulerTest()
            : pc(pl),
              singleChunk({},
                          pc.prepareBlockPages({10},
                                               {{VarLenFullID(
                                                       std::unique_ptr<byte[]>(new byte[4]{0x10, 0x44, 0x5, 0}))}},
                                               {}),
                          {{{app_1_id, chunk1_local_id}},
                           {{8,        3}}},
                          0) {
        singleChunk.getChunk({app_1_id, chunk1_local_id})->setSize(5);
    }
};

class MockGraph {
public:
    MOCK_METHOD(void, registerDependency, (const std::vector<AppRequestIdType>& cluster, AppRequestIdType v));

    MOCK_METHOD(void, registerDependency, (AppRequestIdType u, AppRequestIdType v));

    MOCK_METHOD(void, registerClique, (const std::vector<AppRequestIdType>& clique));
};

class MockDag {
public:
    MOCK_METHOD(bool, isAdjacent, (AppRequestIdType u, AppRequestIdType v), (const));
};

class LoggerDag {
public:
    bool isAdjacent(AppRequestIdType u, AppRequestIdType v) const {
        printf("(%ld, %ld) \n", u, v);
        return true;
    }
};

class LoggerGraph {
public:
    void registerDependency(const std::vector<AppRequestIdType>& cluster, AppRequestIdType v) {
        printf("[ ");
        for (const auto& u: cluster) {
            printf("%ld ", u);
        }
        printf("]->(%ld)\n", v);
    }

    void registerDependency(AppRequestIdType u, AppRequestIdType v) {
        printf("%ld->%ld\n", u, v);
    }

    void registerClique(const std::vector<AppRequestIdType>& clique) {
        printf("(");
        for (int i = 0; i < clique.size() - 1; ++i) {
            printf("%ld ", clique[i]);
        }
        printf("%ld)\n", clique.back());
    }
};

TEST_F(RequestSchedulerTest, CollisionCliques_1) {
    // 0 0 0 0 * * * * * *
    // 1 1 * * * * * * * *
    // * * 2 2 2 2 2 2 * *
    // * * * * * * 3 3 3 3
    // * * * * * * 4 4 4 4
    //
    // [ 0 ] * 1
    // ( 1 )
    // ( 0 2 )
    // ( 2 3 4 )
    MockDag mock;
    //ON_CALL(mock, isAdjacent).WillByDefault(Return(true));
    EXPECT_CALL(mock, isAdjacent(0, 1)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(0, 2)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(2, 3)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(3, 4)).WillOnce(Return(true));

    RequestScheduler::findCollisionCliques<VerifierCluster<MockDag>>(
            {0, 0, 2, 6, 6}, {
                    {4, Access::writable, 0},
                    {2, Access::writable, 1},
                    {6, Access::writable, 2},
                    {4, Access::writable, 3},
                    {4, Access::writable, 4},
            },
            &mock);
}

TEST_F(RequestSchedulerTest, CollisionCliques_2) {
    // ( 0 1 2 3 )
    // ( 2 3 4 5 )

    MockDag mock;
    EXPECT_CALL(mock, isAdjacent(0, 1)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(1, 2)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(2, 3))
            .WillOnce(Return(true))
            .WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(3, 4)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(4, 5)).WillOnce(Return(true));

    RequestScheduler::findCollisionCliques<VerifierCluster<MockDag>>(
            {0, 0, 2, 2, 5, 5},
            {
                    {3, Access::writable, 0},
                    {3, Access::writable, 1},
                    {5, Access::writable, 2},
                    {5, Access::writable, 3},
                    {4, Access::writable, 4},
                    {4, Access::writable, 5},
            },
            &mock);

}

TEST_F(RequestSchedulerTest, CliquetoSingleNode_1) {
    // 1 1 1 1 * * w
    // 3 3 3 3 * * w
    // 4 4 4 4 * * w
    // * * 2 2 * * r

    MockDag mock;
    EXPECT_CALL(mock, isAdjacent(1, 3)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(3, 4)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(1, 2)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(2, 3)).WillOnce(Return(true));

    RequestScheduler::findCollisionCliques<VerifierCluster<MockDag>>(
            {0, 0, 0, 2},
            {
                    {4, Access::writable,  1},
                    {4, Access::writable,  3},
                    {4, Access::writable,  4},
                    {2, Access::read_only, 2},
            },
            &mock);
}

TEST_F(RequestSchedulerTest, CliquetoSingleNode_2) {
    // 1 1 1 1 * * w
    // 3 3 3 3 * * w
    // 4 4 4 4 * * w
    // * * 0 0 * * r

    MockDag mock;
    EXPECT_CALL(mock, isAdjacent(1, 3)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(3, 4)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(0, 1)).WillOnce(Return(true));

    RequestScheduler::findCollisionCliques<VerifierCluster<MockDag>>(
            {0, 0, 0, 2},
            {
                    {4, Access::writable,  1},
                    {4, Access::writable,  3},
                    {4, Access::writable,  4},
                    {2, Access::read_only, 0},
            },
            &mock);
}

TEST_F(RequestSchedulerTest, CollisionCliques_3) {
    // 0 0 0 0 0 0 * * * * w
    // * 1 1 1 1 1 1 * * * w
    // * 2 2 2 2 2 2 * * * w
    // * * * * * 3 3 3 3 * r
    // * * * * * 4 4 4 4 * ch
    // * * * * * * 5 5 5 5 r
    //
    // [ 0 1 2 ] * 3
    // ( 0 1 2 )
    // [ 3 ] * 2
    // [ 3 ] * 1
    // [ 1 2 ] * 5
    // ( 1 2 )

    MockDag mock;
    EXPECT_CALL(mock, isAdjacent(0, 1)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(1, 2))
            .WillOnce(Return(true))
            .WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(2, 3))
            .WillOnce(Return(true))
            .WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(1, 3)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(2, 5)).WillOnce(Return(true));

    RequestScheduler::findCollisionCliques<VerifierCluster<MockDag>>(
            {0, 1, 1, 5, 5, 6},
            {
                    {6, Access::writable,   0},
                    {6, Access::writable,   1},
                    {6, Access::writable,   2},
                    {4, Access::read_only,  3},
                    {4, Access::check_only, 4},
                    {4, Access::read_only,  5},
            },
            &mock);
}

TEST_F(RequestSchedulerTest, CollisionCliques_additive) {
    // * 0 0 0 0 0 0 *
    // * 1 1 1 1 1 1 *
    // * * * * 2 2 2 *
    // * * * * 3 3 3 *
    // * * * * 4 4 4 4
    //
    // [ 0 1 ] * 2
    // [ 0 1 ] * 3
    // [ 0 1 ] * 4
    // [ 2 3 ] * 4

    MockDag mock;
    EXPECT_CALL(mock, isAdjacent(0, 2)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(0, 3)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(0, 4)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(1, 2)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(1, 3)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(1, 4)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(2, 4)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(3, 4)).WillOnce(Return(true));

    RequestScheduler::findCollisionCliques<VerifierCluster<MockDag>>(
            {1, 1, 4, 4, 4},
            {
                    {6, Access::int_additive, 0},
                    {6, Access::int_additive, 1},
                    {3, Access::int_additive, 2},
                    {3, Access::int_additive, 3},
                    {4, Access::int_additive, 4},
            },
            &mock);
}

TEST_F(RequestSchedulerTest, ResizingCollisions) {
    // * * * * 2 2 2 2 2 2 2 * * * expand
    // * * * * * * * 3 3 3 3 3 3 3 shrink
    // 0 0 * * * * * * * * * * * * w
    // * 1 1 1 1 * * * * * * * * * c
    // * * * * * * 4 4 * * * * * * r
    // * * * * * * * * * * * 5 5 * a
    // * * * * 2 2 * * * * * * * * w

    MockDag resizing;
    EXPECT_CALL(resizing, isAdjacent(1, 2)).WillOnce(Return(true));
    EXPECT_CALL(resizing, isAdjacent(2, 4)).WillOnce(Return(true));
    EXPECT_CALL(resizing, isAdjacent(3, 4)).WillOnce(Return(true));
    EXPECT_CALL(resizing, isAdjacent(3, 5)).WillOnce(Return(true));

    MockDag normal;
    EXPECT_CALL(normal, isAdjacent(2, 6)).WillOnce(Return(true));
    EXPECT_CALL(normal, isAdjacent(3, 6)).WillOnce(Return(true));
    EXPECT_CALL(normal, isAdjacent(2, 3)).WillOnce(Return(true));

    vector<int32> offsets({-3, -2, -1, -1, 0, 1, 4, 6, 11});
    vector<AccessBlockInfo> blocks({
                                           {10, Access::writable,     0},
                                           {10, Access::read_only,    6},
                                           {11, Access::writable,     2},
                                           {-7, Access::writable,     3},

                                           {2,  Access::writable,     0},
                                           {4,  Access::check_only,   1},
                                           {2,  Access::writable,     2},
                                           {2,  Access::read_only,    4},
                                           {2,  Access::int_additive, 5},
                                   });

    RequestScheduler::findResizingCollisions<VerifierCluster<MockDag>>(offsets, blocks, &resizing, [] { return 4; });
    RequestScheduler::findCollisionCliques<VerifierCluster<MockDag>>(std::move(offsets), std::move(blocks), &normal);
}

TEST_F(RequestSchedulerTest, SortingRequests) {
    RequestScheduler scheduler(8, singleChunk);

    scheduler.addRequest({.id = 4, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{1}, {{1, Access::read_only, 0},}},
                                 }}}},
                                 .adjList = {}});
    scheduler.addRequest({.id = 3, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{1}, {{1, Access::read_only, 6},}},
                                 }}}},
                                 .adjList = {}});
    scheduler.addRequest({.id = 2, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{1}, {{1, Access::writable, 3},}},
                                 }}}},
                                 .adjList = {}});
    scheduler.addRequest({.id = 1, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{1}, {{1, Access::writable, 7},}},
                                 }}}},
                                 .adjList = {}});
    scheduler.addRequest({.id = 0, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{1}, {{1, Access::writable, 1},}},
                                 }}}},
                                 .adjList = {}});
    scheduler.addRequest({.id = 5, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{1}, {{1, Access::check_only, 5},}},
                                 }}}},
                                 .adjList = {}});
    scheduler.addRequest({.id = 6, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{1}, {{1, Access::int_additive, 4},}},
                                 }}}},
                                 .adjList = {}});
    scheduler.addRequest({.id = 7, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{1}, {{1, Access::int_additive, 2},}},
                                 }}}},
                                 .adjList = {}});


    vector<AccessBlockInfo> want(
            {
                    {1, Access::check_only,   5},
                    {1, Access::writable,     1},
                    {1, Access::writable,     3},
                    {1, Access::writable,     7},
                    {1, Access::read_only,    0},
                    {1, Access::read_only,    6},
                    {1, Access::int_additive, 2},
                    {1, Access::int_additive, 4},
            });
    auto sortedMap = scheduler.sortAccessBlocks(6);

    EXPECT_EQ(sortedMap.at(app_1_id).at(chunk1_local_id).getValues(), want);
}

TEST_F(RequestSchedulerTest, CollisionsFromRequests) {
    // [13--10] [13--11] [10--11] [4--1] [1--2] [2--5] [2--10] [2--11] [5--3] [5--10] [5--11] [3--6] [3--10] [3--11] [6--10] [6--11] [7--11] [8--11]
    RequestScheduler scheduler(14, singleChunk);

    scheduler.addRequest({.id = 12, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{-3}, {{0, Access::read_only, 12},}},
                                 }}}},
                                 .adjList = {}});
    scheduler.addRequest({.id = 13, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{-2}, {{1, Access::read_only, 13},}},
                                 }}}},
                                 .adjList = {}});
    scheduler.addRequest({.id = 10, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{-1}, {{6, Access::writable, 10},}},
                                 }}}},
                                 .adjList = {13, 11}});
    scheduler.addRequest({.id = 11, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{-1}, {{-3, Access::writable, 11},}},
                                 }}}},
                                 .adjList = {13}});
    scheduler.addRequest({.id = 4, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{0}, {{2, Access::read_only, 4},}},
                                 }}}},
                                 .adjList = {}});
    scheduler.addRequest({.id = 1, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{1}, {{2, Access::writable, 1},}},
                                 }}}},
                                 .adjList = {4, 2}});
    scheduler.addRequest({.id = 5, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{2}, {{3, Access::read_only, 5},}},
                                 }}}},
                                 .adjList = {10, 11}});
    scheduler.addRequest({.id = 2, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{2}, {{2, Access::writable, 2},}},
                                 }}}},
                                 .adjList = {5, 10, 11}});
    scheduler.addRequest({.id = 3, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{4}, {{2, Access::writable, 3},}},
                                 }}}},
                                 .adjList = {5, 6, 10, 11}});
    scheduler.addRequest({.id = 6, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{5}, {{1, Access::read_only, 6},}},
                                 }}}},
                                 .adjList = {10, 11}});
    scheduler.addRequest({.id = 7, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{6}, {{2, Access::read_only, 7},}},
                                 }}}},
                                 .adjList = {11}});
    scheduler.addRequest({.id = 8, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{7}, {{1, Access::read_only, 8},}},
                                 }}}},
                                 .adjList = {11}});


    auto sortedMap = scheduler.sortAccessBlocks(8);

    scheduler.checkCollisions({app_1_id, chunk1_local_id},
                              sortedMap.at(app_1_id).at(chunk1_local_id).getKeys(),
                              sortedMap.at(app_1_id).at(chunk1_local_id).getValues());

    // [13--10] [13--11] [10--11] [4--1] [1--2] [2--5] [2--10] [2--11] [5--3] [5--10] [5--11] [3--6] [3--10] [3--11]
    // [6--10] [6--11] [7--11] [8--11]
    MockDag mock;
    EXPECT_CALL(mock, isAdjacent(10, 13)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(11, 13)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(10, 11)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(1, 4)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(1, 2)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(2, 5)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(2, 10)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(2, 11)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(3, 5)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(5, 10)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(5, 11)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(3, 6)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(3, 10)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(3, 11)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(6, 10)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(6, 11)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(7, 11)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(8, 11)).WillOnce(Return(true));

    RequestScheduler::findResizingCollisions<VerifierCluster<MockDag>>(
            sortedMap.at(app_1_id).at(chunk1_local_id).getKeys(),
            sortedMap.at(app_1_id).at(chunk1_local_id).getValues(),
            &mock, [] { return 3; });
    RequestScheduler::findCollisionCliques<VerifierCluster<MockDag>>(
            std::move(sortedMap.at(app_1_id).at(chunk1_local_id).getKeys()),
            std::move(sortedMap.at(app_1_id).at(chunk1_local_id).getValues()),
            &mock);
}

TEST_F(RequestSchedulerTest, AdditiveCollisions) {
    // [0--1] [0--2] [1--3] [2--3] [3--4]
    RequestScheduler scheduler(5, singleChunk);

    scheduler.addRequest({.id = 0, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{-3, 0}, {{0, Access::read_only, 0}, {3, Access::writable, 0}}},
                                 }}}},
                                 .adjList = {1, 2}});
    scheduler.addRequest({.id = 1, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{-3, 2}, {{0, Access::read_only, 1}, {4, Access::int_additive, 1}}},
                                 }}}},
                                 .adjList = {3}});
    scheduler.addRequest({.id = 2, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{-3, 2}, {{0, Access::read_only, 2}, {4, Access::int_additive, 2}}},
                                 }}}},
                                 .adjList = {3}});
    scheduler.addRequest({.id = 3, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{-3, 4}, {{0, Access::read_only, 3}, {3, Access::int_additive, 3}}},
                                 }}}},
                                 .adjList = {4}});
    scheduler.addRequest({.id = 4, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{-3, 6}, {{0, Access::read_only, 4}, {2, Access::read_only, 4}}},
                                 }}}},
                                 .adjList = {}});

    auto sortedMap = scheduler.sortAccessBlocks(4);

    scheduler.checkCollisions({app_1_id, chunk1_local_id},
                              std::move(sortedMap.at(app_1_id).at(chunk1_local_id).getKeys()),
                              std::move(sortedMap.at(app_1_id).at(chunk1_local_id).getValues()));
}


TEST_F(RequestSchedulerTest, WritersCollisions) {
    // 0 0 0 * * * * * w
    // 1 1 1 1 * * * * w
    // 2 2 2 2 * * * * w
    // * * * * 3 3 3 3 w
    // * * * * 4 4 4 4 w

    RequestScheduler scheduler(5, singleChunk);

    scheduler.addRequest({.id = 0, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{0}, {{3, Access::writable, 0}}},
                                 }}}},
                                 .adjList = {1}});
    scheduler.addRequest({.id = 1, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{0}, {{4, Access::writable, 1}}},
                                 }}}},
                                 .adjList = {2}});
    scheduler.addRequest({.id = 2, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{0}, {{4, Access::writable, 2}}},
                                 }}}},
                                 .adjList = {}});
    scheduler.addRequest({.id = 3, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{4}, {{4, Access::writable, 3}}},
                                 }}}},
                                 .adjList = {4}});
    scheduler.addRequest({.id = 4, .memoryAccessMap = {
            {app_1_id},
            {{{chunk1_local_id}, {
                                         {{4}, {{4, Access::writable, 4}}},
                                 }}}},
                                 .adjList = {}});

    auto sortedMap = scheduler.sortAccessBlocks(2);


    scheduler.checkCollisions({app_1_id, chunk1_local_id},
                              sortedMap.at(app_1_id).at(chunk1_local_id).getKeys(),
                              sortedMap.at(app_1_id).at(chunk1_local_id).getValues());


    MockDag mock;
    EXPECT_CALL(mock, isAdjacent(0, 1)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(1, 2)).WillOnce(Return(true));
    EXPECT_CALL(mock, isAdjacent(3, 4)).WillOnce(Return(true));

    RequestScheduler::findCollisionCliques<VerifierCluster<MockDag>>(
            std::move(sortedMap.at(app_1_id).at(chunk1_local_id).getKeys()),
            std::move(sortedMap.at(app_1_id).at(chunk1_local_id).getValues()),
            &mock);
}


TEST_F(RequestSchedulerTest, SimpleDagFull) {
    auto numOfRequests = 3;
    RequestScheduler scheduler(numOfRequests, singleChunk);

    scheduler.addRequest(
            {
                    .id = 0,
                    .memoryAccessMap = {
                            {app_1_id},
                            {{{chunk1_local_id}, {
                                                         {{-2, 3},
                                                                 {{1, Access::read_only, 0},
                                                                         {2, Access::writable, 0}}},
                                                 }}}},
                    .adjList = {2}
            });

    scheduler.addRequest(
            {
                    .id = 1,
                    .memoryAccessMap = {
                            {app_1_id},
                            {{{chunk1_local_id}, {
                                                         {{-2, 0},
                                                                 {{1, Access::read_only, 1},
                                                                         {1, Access::read_only, 1}}},
                                                 }}}},
                    .adjList = {2}
            });

    scheduler.addRequest(
            {
                    .id = 2,
                    .memoryAccessMap = {
                            {app_1_id},
                            {{{chunk1_local_id}, {
                                                         {{-2, 0, 3},
                                                                 {{1, Access::read_only, 2},
                                                                         {1, Access::int_additive, 2},
                                                                         {2, Access::read_only, 2}}},
                                                 }}}},
                    .adjList = {}
            });

    auto sortedMap = scheduler.sortAccessBlocks(8);

    scheduler.checkCollisions({app_1_id, chunk1_local_id}, sortedMap.at(app_1_id).at(chunk1_local_id).getKeys(),
                              sortedMap.at(app_1_id).at(chunk1_local_id).getValues());

    for (int i = 0; i < numOfRequests; ++i) {
        scheduler.finalizeRequest(i);
    }

    scheduler.buildExecDag();

    std::vector<AppRequestIdType> result;
    while (auto* next = scheduler.nextRequest()) {
        result.emplace_back(next->id);
        std::cout << next->id << std::endl;
        scheduler.submitResult(next->id, 200);
    }

    EXPECT_EQ(result, std::vector<AppRequestIdType>({0, 1, 2}));
}

TEST_F(RequestSchedulerTest, ExecutionDag) {
    struct DagTester {
        RequestScheduler scheduler;
        std::vector<AppRequestIdType> want;
        int n;
        bool wantError;

        DagTester(
                int n,
                ChunkIndex& index,
                std::vector<AppRequestInfo> nodeData,
                std::vector<AppRequestIdType> want,
                bool wantError = false
        ) : scheduler(n, index), want(std::move(want)), n(n), wantError(wantError) {
            for (int i = 0; i < n; ++i) {
                scheduler.addRequest(std::move(nodeData.at(i)));
            }
        }

        void test() {
            for (int i = 0; i < n; ++i) {
                scheduler.finalizeRequest(i);
            }

            scheduler.buildExecDag();

            if (wantError) {
                EXPECT_THROW(while (auto* next = scheduler.nextRequest()) scheduler.submitResult(next->id, 200),
                             BlockError);
            } else {
                std::vector<AppRequestIdType> got;
                while (auto* next = scheduler.nextRequest()) {
                    got.emplace_back(next->id);
                    std::cout << next->id << std::endl;
                    scheduler.submitResult(next->id, 200);
                }
                EXPECT_EQ(got, want);
            }
        }
    };

    //      4 --> 1
    //    / ^     |
    //  0   |     v
    //    \ 2 --> 3
    DagTester t1(5, singleChunk,
                 {
                         {.id = 0, .adjList = {4, 2}},
                         {.id = 3, .adjList = {}},
                         {.id = 2, .adjList = {3, 4}},
                         {.id = 1, .adjList = {3}},
                         {.id = 4, .adjList = {1}}
                 },
                 {0, 2, 4, 1, 3}
    );
    SUB_TEST("Simple DAG", t1);

    // 0 --> 2 <-- 3
    //        \    ^
    //         \   |
    //          v  1
    DagTester t2(4, singleChunk,
                 {
                         {.id = 0, .adjList = {2}},
                         {.id = 3, .adjList = {2}},
                         {.id = 1, .adjList = {3}},
                         {.id = 2, .adjList = {1}},
                 },
                 {},
                 true
    );
    SUB_TEST("Simple loop detection", t2);

    DagTester t3(3, singleChunk,
                 {
                         {.id = 0, .adjList = {}},
                         {.id = 2, .adjList = {}},
                         {.id = 1, .adjList = {}},
                 },
                 {0, 1, 2}
    );
    SUB_TEST("Empty DAG", t3);

    DagTester t4(3, singleChunk,
                 {
                         {.id = 0, .adjList = {}},
                         {.id = 2, .adjList = {}},
                         {.id = 1, .adjList = {2}},
                 },
                 {0, 1, 2}
    );
    SUB_TEST("Two source nodes", t4);

    DagTester t5(3, singleChunk,
                 {
                         {.id = 0, .adjList = {}},
                         {.id = 2, .adjList = {1}},
                         {.id = 1, .adjList = {}},
                 },
                 {},
                 true
    );
    SUB_TEST("Wrong source nodes", t5);

    DagTester t6(4, singleChunk,
                 {
                         {.id = 3, .adjList ={1, 2}},
                         {.id = 0, .adjList ={3}},
                         {.id = 2, .adjList ={}},
                         {.id = 1, .adjList ={}},
                 },
                 {0, 3, 2, 1}
    );
    SUB_TEST("Wrong source nodes", t6);
}

/*
TEST(RequestSchedulerTest, SimpleCollisionDetection) {
    PageLoader pl{};
    heap::PageCache pc(pl);
    heap::PageCache::ChunkIndex index(pc, {app_1_id}, {{100, true}},
                                      {{100},
                                       {{8, 3}}});


    RequestScheduler scheduler(14, index);
    scheduler.findCollisions(100, {-3, -2, -1, -1, 0, 1, 2, 2, 4, 5, 6}, {
            {0,  Access::read_only, 12},     // -3
            {1,  Access::read_only, 13},     // -2
            {6,  Access::writable,  10},     // -1
            {-3, Access::writable,  11},     // -1
            {2,  Access::read_only, 4},     // 0
            {2,  Access::writable,  1},     // 1
            {3,  Access::read_only, 5},     // 2
            {2,  Access::writable,  2},     // 2
            {2,  Access::writable,  3},     // 4
            {1,  Access::read_only, 6},     // 5
            {2,  Access::read_only, 7},     // 6
            //  {1,  Access::read_only, 8},     // 8

            // [10->13] [11->13] [10->11] [1->4] [1->5] [1->2] [2->5] [3->5] [5->10] [5->11] [2->10] [2->11] [3->6] [3->10] [3->11] [6->10] [6->11] [7->11]
    });
}
*/