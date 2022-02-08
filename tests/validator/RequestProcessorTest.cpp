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
constexpr long_id app_1_id(0x1000000000000000);
constexpr int max_workers_count = 32;

class RequestProcessorTest : public ::testing::Test {
protected:
    PageLoader pl{};
    PageCache pc;
    ChunkIndex singleChunk;

public:
    RequestProcessorTest()
            : pc(pl),
              singleChunk(
                      pc.prepareBlockPages({10},
                                           {{VarLenFullID(std::unique_ptr<byte[]>(new byte[4]{0x10, 0x44, 0x5, 0})),
                                             true}},
                                           {}),
                      {{{app_1_id, chunk1_local_id}},
                       {{8,        3}}},
                      0) {
        singleChunk.getChunk({app_1_id, chunk1_local_id})->setSize(5);
    }
};

class MockExecutor {
public:
    AppResponse executeOne(AppRequest* req) const {
        std::cout << "->" << req->id;
        return executeOne(req->id);
    }

    MOCK_METHOD(AppResponse, executeOne, (AppRequestIdType id), (const));
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
        RequestProcessor rp(singleChunk, int(requests.size()), workers);

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

        rp.executeRequests(mock);
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

        RequestProcessor rp(singleChunk, int(requests.size()), 5);

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

        rp.executeRequests(mock);
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

        RequestProcessor rp(singleChunk, int(requests.size()), 5);

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

        EXPECT_THROW(rp.executeRequests(mock), BlockError);
        std::cout << std::endl;
    }
}
