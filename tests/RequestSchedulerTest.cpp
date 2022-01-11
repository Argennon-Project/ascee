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
#include "executor/RequestScheduler.h"

using namespace ascee;
using namespace runtime;


using Access = BlockAccessInfo::Type;

class RequestSchedulerTest : public ::testing::Test {
protected:
    PageLoader pl{};
    heap::PageCache pc;
    heap::ChunkIndex singleChunk;

public:
    RequestSchedulerTest()
            : pc(pl),
              singleChunk(pc.prepareBlockPages({10}, {{{10, 100}, true}}, {}),
                          {{{10, 100}},
                           {{8,  3}}}, 0) {
        singleChunk.getChunk({10, 100})->setSize(5);
    }
};

TEST_F(RequestSchedulerTest, CollisionsFromRequests) {
    RequestScheduler scheduler(14, singleChunk);

    scheduler.addRequest({.id = 12, .memoryAccessMap = {
            {10},
            {{{100}, {
                             {{-3}, {{0, Access::read_only, 12},}},
                     }}}},
                                 .adjList = {}});
    scheduler.addRequest({.id = 13, .memoryAccessMap = {
            {10},
            {{{100}, {
                             {{-2}, {{1, Access::read_only, 13},}},
                     }}}},
                                 .adjList = {}});
    scheduler.addRequest({.id = 10, .memoryAccessMap = {
            {10},
            {{{100}, {
                             {{-1}, {{6, Access::writable, 10},}},
                     }}}},
                                 .adjList = {13, 11}});
    scheduler.addRequest({.id = 11, .memoryAccessMap = {
            {10},
            {{{100}, {
                             {{-1}, {{-3, Access::writable, 11},}},
                     }}}},
                                 .adjList = {13}});
    scheduler.addRequest({.id = 4, .memoryAccessMap = {
            {10},
            {{{100}, {
                             {{0}, {{2, Access::read_only, 4},}},
                     }}}},
                                 .adjList = {}});
    scheduler.addRequest({.id = 1, .memoryAccessMap = {
            {10},
            {{{100}, {
                             {{1}, {{2, Access::writable, 1},}},
                     }}}},
                                 .adjList = {4, 2, 5}});
    scheduler.addRequest({.id = 5, .memoryAccessMap = {
            {10},
            {{{100}, {
                             {{2}, {{3, Access::read_only, 5},}},
                     }}}},
                                 .adjList = {10, 11}});
    scheduler.addRequest({.id = 2, .memoryAccessMap = {
            {10},
            {{{100}, {
                             {{2}, {{2, Access::writable, 2},}},
                     }}}},
                                 .adjList = {5, 10, 11}});
    scheduler.addRequest({.id = 3, .memoryAccessMap = {
            {10},
            {{{100}, {
                             {{4}, {{2, Access::writable, 3},}},
                     }}}},
                                 .adjList = {5, 6, 10, 11}});
    scheduler.addRequest({.id = 6, .memoryAccessMap = {
            {10},
            {{{100}, {
                             {{5}, {{1, Access::read_only, 6},}},
                     }}}},
                                 .adjList = {10, 11}});
    scheduler.addRequest({.id = 7, .memoryAccessMap = {
            {10},
            {{{100}, {
                             {{6}, {{2, Access::read_only, 7},}},
                     }}}},
                                 .adjList = {11}});
    scheduler.addRequest({.id = 8, .memoryAccessMap = {
            {10},
            {{{100}, {
                             {{7}, {{1, Access::read_only, 8},}},
                     }}}},
                                 .adjList = {11}});


    auto sortedMap = scheduler.sortAccessBlocks();

    scheduler.findCollisions({10, 100}, sortedMap.at(10).at(100).getKeys(), sortedMap.at(10).at(100).getValues());
}

TEST_F(RequestSchedulerTest, SimpleDagFull) {
    auto numOfRequests = 3;
    RequestScheduler scheduler(numOfRequests, singleChunk);

    scheduler.addRequest(
            {
                    .id = 0,
                    .memoryAccessMap = {
                            {10},
                            {{{100}, {
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
                            {10},
                            {{{100}, {
                                             {{-2, 0},
                                                     {{1, Access::read_only, 1},
                                                             {1, Access::read_only, 1}}},
                                     }}}},
                    .adjList = {}
            });

    scheduler.addRequest(
            {
                    .id = 2,
                    .memoryAccessMap = {
                            {10},
                            {{{100}, {
                                             {{-2, 0, 3},
                                                     {{1, Access::read_only, 2},
                                                             {1, Access::int_additive, 2},
                                                             {2, Access::read_only, 2}}},
                                     }}}},
                    .adjList = {1}
            });

    auto sortedMap = scheduler.sortAccessBlocks();

    scheduler.findCollisions({10, 100}, sortedMap.at(10).at(100).getKeys(), sortedMap.at(10).at(100).getValues());

    for (int i = 0; i < numOfRequests; ++i) {
        scheduler.finalizeRequest(i);
    }

    scheduler.buildExecDag();

    std::vector<AppRequestIdType> result;
    while (auto* next = scheduler.nextRequest()) {
        result.emplace_back(next->id);
        std::cout << next->id << std::endl;
        scheduler.submitResult({next->id, 200, ""});
    }

    EXPECT_EQ(result, std::vector<AppRequestIdType>({0, 2, 1}));
}

TEST_F(RequestSchedulerTest, ExecutionDag) {
    struct DagTester {
        RequestScheduler scheduler;
        std::vector<AppRequestIdType> want;
        int n;
        bool wantError;

        DagTester(
                int n,
                heap::ChunkIndex& index,
                std::vector<AppRequestRawData> nodeData,
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
                EXPECT_THROW(while (auto* next = scheduler.nextRequest()) scheduler.submitResult({next->id, 200, ""}),
                             BlockError);
            } else {
                std::vector<AppRequestIdType> got;
                while (auto* next = scheduler.nextRequest()) {
                    got.emplace_back(next->id);
                    std::cout << next->id << std::endl;
                    scheduler.submitResult({next->id, 200, ""});
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
}

/*
TEST(RequestSchedulerTest, SimpleCollisionDetection) {
    PageLoader pl{};
    heap::PageCache pc(pl);
    heap::PageCache::ChunkIndex index(pc, {10}, {{100, true}},
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