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

#include "validator/RequestProcessor.hpp"
#include "subtest.h"
#include "storage/PageLoader.h"
#include "storage/PageCache.h"
#include <gmock/gmock.h>

using namespace argennon;
using namespace ave;
using namespace asa;
using namespace ascee::runtime;
using testing::Sequence;

using Access = AccessBlockInfo::Access::Type;

constexpr long_long_id chunk1_local_id(0x4400000000000000, 0x0500000000000000);
constexpr long_long_id chunk2_local_id(0x4400000000000000, 0x0400000000000000);
constexpr long_id app_1_id(0x1000000000000000);
constexpr int max_workers_count = 32;

class RequestProcessorTest : public ::testing::Test {
protected:
    PageLoader pl{};
    PageCache pc;
    ChunkIndex singleChunk;
    AppLoader loader{"apps"};
    AppIndex appIndex{&loader};

public:
    RequestProcessorTest()
            : pc(pl),
              singleChunk({},
                          pc.preparePages({10},
                                          {{VarLenFullID(
                                                  std::unique_ptr<byte[]>(new byte[4]{0x10, 0x44, 0x5, 0}))}},
                                          {}),
                          {{{app_1_id, chunk1_local_id}},
                           {{8,        3}}},
                          0) {
        singleChunk.getChunk({app_1_id, chunk1_local_id})->setSize(5);
        appIndex.prepareApps({123}, {arg_app_id_g});
    }
};

class MockExecutor {
public:
    MOCK_METHOD(AppResponse, executeOne, (AppRequestIdType id), (const));
};

class FakeExecutor {
public:
    static inline MockExecutor* mock;

    AppResponse executeOne(AppRequest* req) const {
        std::cout << "->" << req->id;
        return mock->executeOne(req->id);
    }
};

class FakeStream {
public:
    class EndOfStream : std::exception {
    };

    FakeStream(int start, int end, std::vector<AppRequestInfo>& requests) : current(start), end(end),
                                                                            requests(requests) {}

    AppRequestInfo next() {
        if (current >= end) throw EndOfStream();
        return requests.at(current++);
    }

private:
    int current;
    int end;
    std::vector<AppRequestInfo>& requests;
};

TEST_F(RequestProcessorTest, ExecOrderTest_1) {
    for (int workers = 0; workers < max_workers_count; ++workers) {
        std::vector<AppRequestInfo> requests{
                {.id = 3, .adjList ={1, 2}},
                {.id = 0, .adjList ={3}},
                {.id = 2, .adjList ={}},
                {.id = 1, .adjList ={}},
        };
        RequestProcessor rp(singleChunk, appIndex, int(requests.size()), workers);

        rp.loadRequests<FakeStream>({
                                            {0, 1, requests},
                                            {1, 4, requests},
                                    });
        MockExecutor mock;
        Sequence s1, s2;
        EXPECT_CALL(mock, executeOne(0)).InSequence(s1, s2);
        EXPECT_CALL(mock, executeOne(3)).InSequence(s1, s2);
        EXPECT_CALL(mock, executeOne(2)).InSequence(s2);
        EXPECT_CALL(mock, executeOne(1)).InSequence(s1);

        FakeExecutor::mock = &mock;
        rp.executeRequests<FakeExecutor>();
        std::cout << std::endl;
    }
}

TEST_F(RequestProcessorTest, ExecOrderTest_2) {
    for (int workers = 0; workers < max_workers_count; ++workers) {
        std::vector<AppRequestInfo> requests{
                {.id = 5, .adjList ={}},
                {.id = 4, .adjList ={5}},
                {.id = 3, .adjList ={4}},
                {.id = 2, .adjList ={4}},
                {.id = 1, .adjList ={4}},
                {.id = 0, .adjList ={1, 2, 3}},
        };

        RequestProcessor rp(singleChunk, appIndex, int(requests.size()), 5);

        rp.loadRequests<FakeStream>({
                                            {0, 1, requests},
                                            {1, 2, requests},
                                            {2, 3, requests},
                                            {3, 6, requests},
                                    });
        MockExecutor mock;
        Sequence s1, s2, s3;
        EXPECT_CALL(mock, executeOne(0)).InSequence(s1, s2, s3);
        EXPECT_CALL(mock, executeOne(1)).InSequence(s1);
        EXPECT_CALL(mock, executeOne(2)).InSequence(s2);
        EXPECT_CALL(mock, executeOne(3)).InSequence(s3);
        EXPECT_CALL(mock, executeOne(4)).InSequence(s1, s2, s3);
        EXPECT_CALL(mock, executeOne(5)).InSequence(s1);


        FakeExecutor::mock = &mock;
        rp.executeRequests<FakeExecutor>();
        std::cout << std::endl;
    }
}


TEST_F(RequestProcessorTest, ExecOrderTest_loop) {
    for (int workers = 0; workers < max_workers_count; ++workers) {
        std::vector<AppRequestInfo> requests{
                {.id = 5, .adjList ={2}},
                {.id = 4, .adjList ={5}},
                {.id = 3, .adjList ={4}},
                {.id = 2, .adjList ={3}},
                {.id = 1, .adjList ={2, 5}},
                {.id = 0, .adjList ={1}},
        };

        RequestProcessor rp(singleChunk, appIndex, int(requests.size()), 5);

        rp.loadRequests<FakeStream>({
                                            {0, 1, requests},
                                            {1, 2, requests},
                                            {2, 3, requests},
                                            {3, 6, requests},
                                    });
        MockExecutor mock;
        Sequence s1;
        EXPECT_CALL(mock, executeOne(0)).InSequence(s1);
        EXPECT_CALL(mock, executeOne(1)).InSequence(s1);


        FakeExecutor::mock = &mock;

        EXPECT_THROW(rp.executeRequests<FakeExecutor>(), BlockError);
        std::cout << std::endl;
    }
}

TEST_F(RequestProcessorTest, SimpleDependencyGraph) {
    // 0 0 0 * * * * w
    // * * 1 1 1 1 * r
    // * * * 2 2 2 2 r
    //
    // 3 3 3 3 * * * a
    // 4 4 4 4 * * * a
    // * * * 5 5 * * r
    std::vector<AppRequestInfo> requests{
            {
                    .id = 0,
                    .memoryAccessMap = {
                            {app_1_id},
                            {{{chunk1_local_id}, {{{0}, {{3, Access::writable, 0}}},}}}},
                    .adjList ={1}
            },
            {
                    .id = 1,
                    .memoryAccessMap = {
                            {app_1_id},
                            {{{chunk1_local_id}, {{{2}, {{4, Access::read_only, 1}}},}}}},
                    .adjList ={}
            },
            {
                    .id = 2,
                    .memoryAccessMap = {
                            {app_1_id},
                            {{{chunk1_local_id}, {{{3}, {{4, Access::read_only, 2}}},}}}},
                    .adjList ={}
            },
            {
                    .id = 3,
                    .memoryAccessMap = {
                            {app_1_id},
                            {{{chunk2_local_id}, {{{0}, {{4, Access::int_additive, 3}}},}}}},
                    .adjList ={5}
            },
            {
                    .id = 4,
                    .memoryAccessMap = {
                            {app_1_id},
                            {{{chunk2_local_id}, {{{0}, {{4, Access::int_additive, 4}}},}}}},
                    .adjList ={5}
            },
            {
                    .id = 5,
                    .memoryAccessMap = {
                            {app_1_id},
                            {{{chunk2_local_id}, {{{3}, {{2, Access::read_only, 5}}},}}}},
                    .adjList ={}
            },
    };


    Page p1(123);
    Page p2(123);

    ChunkIndex index(
            {},
            {{{app_1_id, chunk1_local_id}, &p1},
             {{app_1_id, chunk2_local_id}, &p2}},
            {{{app_1_id, chunk1_local_id}, {app_1_id, chunk2_local_id}},
             {{15,       0},               {15,       0}}},
            5);

    RequestProcessor rp(index, appIndex, int(requests.size()), 5);

    rp.loadRequests<FakeStream>({
                                        {0, 1, requests},
                                        {1, 3, requests},
                                        {3, 6, requests},
                                });

    rp.buildDependencyGraph();
}
