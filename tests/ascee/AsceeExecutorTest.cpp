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

#include "executor/Executor.h"
#include "argc/types.h"
#include "subtest.h"
#include "storage/AppLoader.h"
#include "storage/AppIndex.h"

#define NORMAL_GAS 1500
#define LOW_GAS 100

using namespace argennon;
using namespace ascee;
using namespace runtime;
using std::string, std::vector, std::to_string, ::testing::Return, ::testing::AtLeast, ::testing::AtMost;

class AsceeExecutorTest : public ::testing::Test {
protected:
public:
    AsceeExecutorTest() {}
};

struct AppTestCase {
    std::string_view libPath;
    long_id calledApp;
    string request;
    int_fast32_t gas;
    vector<long_id> appAccessList;
    string wantResponse;
    int wantCode;
    vector<std::pair<std::string_view, uint64_t>> wantCalls;


    void test() const {
        asa::AppLoader loader(libPath);
        asa::AppIndex appIndex(&loader);

        vector<long_id> shiftedAccessList;
        for (auto& appID: appAccessList) {
            shiftedAccessList.emplace_back(appID << 56);
        }

        appIndex.prepareApps({123}, shiftedAccessList);
        AppRequest req{
                .calledAppID = calledApp << 56,
                .httpRequest = request,
                .gas = gas,
                .modifier = argennon::mocking::ascee::MockModifier(),
                .appTable = appIndex.buildAppTable(shiftedAccessList),
        };
        {
            testing::InSequence seq;
            for (const auto call: wantCalls) {
                if (call.first.starts_with("s")) {
                    EXPECT_CALL(req.modifier, saveVersion()).WillOnce(Return(call.second));
                } else if (call.first.starts_with("l")) {
                    EXPECT_CALL(req.modifier, loadContext(long_id(call.second << 56))).Times(AtLeast(1));
                } else if (call.first.starts_with("*l")) {
                    EXPECT_CALL(req.modifier, loadContext(long_id(call.second << 56))).Times(testing::AnyNumber());
                } else {
                    EXPECT_CALL(req.modifier, restoreVersion(call.second)).Times(1);
                }
            }
        }
        Executor executor;
        auto result = executor.executeOne(&req);

        EXPECT_EQ(result.httpResponse, wantResponse);

        EXPECT_EQ(result.statusCode, wantCode);
    }
};

static
long_id toLongID(uint64_t id) {
    return {id << 56};
}

TEST_F(AsceeExecutorTest, ZeroGas) {
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .libPath = "testdata/single-thread/call",
            .calledApp = 11,
            .request = "test request",
            .gas = 1,
            .appAccessList = {11},
            .wantResponse = string(Executor::Error(
                    "forwarded gas is too low",
                    StatusCode::invalid_operation,
                    toLongID(0)).toHttpResponse(buf)),
            .wantCode = int(StatusCode::invalid_operation),
    };
    SUB_TEST("zero gas", testCase);
}

TEST_F(AsceeExecutorTest, OneLevelCall) {
    AppTestCase testCase{
            .libPath = "testdata/single-thread/call",
            .calledApp = 11,
            .request = "test request",
            .gas = NORMAL_GAS,
            .appAccessList = {11},
            .wantResponse = "test request is DONE!",
            .wantCode = 200,
            .wantCalls = {
                    {"save", 0},
                    {"load", 11},
                    {"load", 0},
            }
    };
    SUB_TEST("one level call", testCase);
}

TEST_F(AsceeExecutorTest, TwoLevelCall) {
    AppTestCase testCase{
            .libPath = "testdata/single-thread/call",
            .calledApp = 15,
            .request = "test request",
            .gas = NORMAL_GAS,
            .appAccessList = {11, 15},
            .wantResponse = "request from 15 is DONE! got in 15",
            .wantCode = 200,
            .wantCalls = {
                    {"save", 0},
                    {"load", 15},
                    {"save", 1},
                    {"load", 11},
                    {"load", 15},
                    {"load", 0},
            }
    };
    SUB_TEST("two level call", testCase);
}

TEST_F(AsceeExecutorTest, ZeroGasTwoLevelCall) {
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .libPath = "testdata/single-thread/call",
            .calledApp = 15,
            .request = "test request",
            .gas = 3,
            .appAccessList = {11, 15},
            .wantResponse = string(Executor::Error(
                    "forwarded gas is too low",
                    StatusCode::invalid_operation,
                    toLongID(15)).toHttpResponse(buf)) + " got in 15",
            .wantCode = 200,
            .wantCalls = {
                    {"save", 0},
                    {"load", 15},
                    {"load", 0},
            }
    };
    SUB_TEST("one level call", testCase);
}

TEST_F(AsceeExecutorTest, AppNotFound) {
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .libPath = "testdata/single-thread/call",
            .calledApp = 16,
            .request = "test request",
            .gas = NORMAL_GAS,
            .appAccessList = {16, 0x50},
            .wantResponse = string(Executor::Error(
                    "app does not exist",
                    StatusCode::not_found,
                    toLongID(16)).toHttpResponse(buf)) + " wrong app!",
            .wantCode = 200,
            .wantCalls = {
                    {"save",    0},
                    {"load",    16},
                    {"save",    1},
                    {"restore", 1},
                    {"load",    16},
                    {"load",    0},
            }
    };
    SUB_TEST("", testCase);
}

TEST_F(AsceeExecutorTest, AppNotDeclared) {
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .libPath = "testdata/single-thread/call",
            .calledApp = 15,
            .request = "test request",
            .gas = NORMAL_GAS,
            .appAccessList = {15},
            .wantResponse = string(Executor::Error(
                    "app/0xb00000000000000 was not declared in the call list",
                    StatusCode::limit_violated,
                    toLongID(15)).toHttpResponse(buf)) + " got in 15",
            .wantCode = 200,
            .wantCalls = {
                    {"save",    0},
                    {"load",    15},
                    {"save",    1},
                    {"restore", 1},
                    {"load",    15},
                    {"load",    0},
            }
    };

    SUB_TEST("not declared", testCase);
}

TEST_F(AsceeExecutorTest, SimpleTimeOut) {
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .libPath = "testdata/single-thread/timeout",
            .calledApp = 10,
            .request = "test request",
            .gas = NORMAL_GAS,
            .appAccessList = {10},
            .wantResponse = string(Executor::Error(
                    "cpu timer expired",
                    StatusCode::execution_timeout,
                    toLongID(10)).toHttpResponse(buf)),
            .wantCode = 421,
            .wantCalls = {
                    {"save",    0},
                    {"load",    10},
                    {"load",    0},
                    {"restore", 0},
                    {"*load",   0},
            }
    };
    SUB_TEST("time out", testCase);
}

TEST_F(AsceeExecutorTest, CalledTimeOut) {
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .libPath = "testdata/single-thread/timeout",
            .calledApp = 12,
            .request = "test request",
            .gas = NORMAL_GAS,
            .appAccessList = {10, 12}, // must be sorted
            .wantResponse = string(Executor::Error(
                    "cpu timer expired",
                    StatusCode::execution_timeout,
                    toLongID(10)).toHttpResponse(buf)) + " TOO LONG...",
            .wantCode = 200,
            .wantCalls = {
                    {"save",    0},
                    {"load",    12},
                    {"save",    1},
                    {"load",    10},
                    {"load",    12},
                    {"restore", 1},
                    {"*load",   12},
                    {"load",    0},
            }
    };
    SUB_TEST("time out", testCase);
}

TEST_F(AsceeExecutorTest, SimpleStackOverflow) {
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .libPath = "testdata/single-thread/stack-overflow",
            .calledApp = 13,
            .request = "test request",
            .gas = NORMAL_GAS,
            .appAccessList = {13},
            .wantResponse = string(Executor::Error(
                    "segmentation fault (possibly stack overflow)",
                    StatusCode::memory_fault,
                    toLongID(13)).toHttpResponse(buf)),
            .wantCode = (int) StatusCode::memory_fault,
            .wantCalls = {
                    {"save",    0},
                    {"load",    13},
                    {"load",    0},
                    {"restore", 0},
                    {"*load",   0},
            }
    };
    SUB_TEST("stack overflow", testCase);
}

TEST_F(AsceeExecutorTest, CalledStackOverflow) {
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .libPath = "testdata/single-thread/stack-overflow",
            .calledApp = 14,
            .request = "test request",
            .gas = NORMAL_GAS,
            .appAccessList = {13, 14},
            .wantResponse = string(Executor::Error(
                    "segmentation fault (possibly stack overflow)",
                    StatusCode::memory_fault,
                    toLongID(13)).toHttpResponse(buf)) + " OVER FLOW... fib: 832040",
            .wantCode = 200,
            .wantCalls = {
                    {"save",    0},
                    {"load",    14},
                    {"save",    1},
                    {"load",    13},
                    {"load",    14},
                    {"restore", 1},
                    {"*load",   14},
                    {"load",    0},
            }
    };
    SUB_TEST("stack overflow", testCase);
}

TEST_F(AsceeExecutorTest, CircularCallLowGas) {
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .libPath = "testdata/single-thread/circular",
            .calledApp = 17,
            .request = "test request",
            .gas = LOW_GAS,
            .appAccessList = {17, 18},
            .wantResponse = string(Executor::Error(
                    "forwarded gas is too low",
                    StatusCode::invalid_operation,
                    toLongID(18)).toHttpResponse(buf)),
            .wantCode = 200
    };
    SUB_TEST("circular", testCase);
}

TEST_F(AsceeExecutorTest, CircularCallHighGas) {
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .libPath = "testdata/single-thread/circular",
            .calledApp = 17,
            .request = "test request",
            .gas = 1000000000,
            .appAccessList = {17, 18},
            .wantResponse = string(Executor::Error(
                    "max call depth reached",
                    StatusCode::limit_exceeded,
                    toLongID(17)).toHttpResponse(buf)),
            .wantCode = 200
    };
    SUB_TEST("circular", testCase);
}

TEST_F(AsceeExecutorTest, FailedCalls) {
    StringBuffer<1024> buf;
    Executor::Error(
            "cpu timer expired",
            StatusCode::execution_timeout,
            toLongID(10)).toHttpResponse(buf);
    Executor::Error(
            "SIGFPE was caught",
            StatusCode::arithmetic_error,
            toLongID(20)).toHttpResponse(buf);
    Executor::Error(
            "calling self",
            StatusCode::invalid_operation,
            toLongID(19)).toHttpResponse(buf);
    Executor::Error(
            "app/0xb00000000000000 was not declared in the call list",
            StatusCode::limit_violated,
            toLongID(19)).toHttpResponse(buf);
    Executor::Error(
            "append: str is too long",
            StatusCode::out_of_range,
            toLongID(21)).toHttpResponse(buf);
    Executor::Error(
            "array::at: __n (which is 6) >= _Nm (which is 6)",
            StatusCode::out_of_range,
            toLongID(24)).toHttpResponse(buf);

    AppTestCase testCase{
            .libPath = "testdata/single-thread/failed-call",
            .calledApp = 19,
            .request = "test request",
            .gas = NORMAL_GAS,
            .appAccessList = {10, 19, 20, 21, 24},
            .wantResponse = string(buf) + " all called...",
            .wantCode = 200,
            .wantCalls = {
                    {"save",    0},
                    {"load",    19},

                    {"save",    1},
                    {"load",    10},
                    {"load",    19},
                    {"restore", 1},
                    {"*load",   19},

                    {"save",    1},
                    {"load",    20},
                    {"load",    19},
                    {"restore", 1},
                    {"*load",   19},

                    {"save",    1},
                    {"restore", 1},
                    {"*load",   19},

                    {"save",    1},
                    {"restore", 1},
                    {"*load",   19},

                    {"save",    1},
                    {"load",    21},
                    {"load",    19},
                    {"restore", 1},
                    {"*load",   19},

                    {"save",    1},
                    {"load",    24},
                    {"load",    19},
                    {"restore", 1},
                    {"*load",   19},

                    {"load",    0},
            },
    };
    SUB_TEST("failed calls", testCase);
}

TEST_F(AsceeExecutorTest, SimpleReentancy) {
    auto reentrancyErr = Executor::Error(
            "reentrancy is not allowed",
            StatusCode::reentrancy_attempt,
            toLongID(22));
    StringBuffer<1024> buf;
    reentrancyErr.toHttpResponse(buf);
    buf.append(" Done: 2");
    reentrancyErr.toHttpResponse(buf);
    reentrancyErr.toHttpResponse(buf);

    AppTestCase testCase{
            .libPath = "testdata/single-thread/reentrancy",
            .calledApp = 22,
            .request = "choice: 1",
            .gas = NORMAL_GAS,
            .appAccessList = {22, 23},
            .wantResponse = string(buf) + " Done: 1",
            .wantCode = 200,
            .wantCalls = {
                    {"save",    0},
                    {"load",    22},

                    {"save",    1},
                    {"load",    23},
                    {"save",    2},
                    {"load",    22},
                    {"load",    23},
                    {"restore", 2},
                    {"*load",   23},
                    {"load",    22},

                    {"save",    2},
                    {"load",    23},
                    {"save",    3},
                    {"load",    22},
                    {"load",    23},
                    {"load",    22},

                    {"save",    4},
                    {"load",    23},
                    {"save",    5},
                    {"load",    22},
                    {"load",    23},
                    {"restore", 5},
                    {"*load",   23},
                    {"load",    22},

                    {"save",    5},
                    {"load",    23},
                    {"save",    6},
                    {"load",    22},
                    {"load",    23},
                    {"restore", 6},
                    {"*load",   23},
                    {"load",    22},

                    {"load",    0},
            },
    };
    SUB_TEST("", testCase);
}

TEST_F(AsceeExecutorTest, SimpleDeferredCall) {
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .libPath = "testdata/single-thread/reentrancy",
            .calledApp = 22,
            .request = "choice: 4",
            .gas = NORMAL_GAS,
            .appAccessList = {22, 23},
            .wantResponse = string(Executor::Error(
                    "deferred success!",
                    StatusCode::bad_request,
                    toLongID(23)).toHttpResponse(buf)),
            .wantCode = (int) StatusCode::bad_request
    };
    SUB_TEST("deferred", testCase);
}
