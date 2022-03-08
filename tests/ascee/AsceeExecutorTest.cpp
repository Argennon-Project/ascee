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
#include "loader/AppLoader.h"
#include "argc/types.h"
#include "subtest.h"

#include <gtest/gtest.h>

#define NORMAL_GAS 1500
#define LOW_GAS 100

using namespace argennon;
using namespace ascee;
using namespace runtime;
using std::string, std::vector, std::to_string;

class AsceeExecutorTest : public ::testing::Test {
protected:
public:
    AsceeExecutorTest() {
        AppLoader::global = std::make_unique<AppLoader>("testdata/single-thread");
    }
};

struct AppTestCase {
    long_id calledApp;
    string request;
    int_fast32_t gas;
    vector<long_id> appAccessList;
    string wantResponse;
    int wantCode;


    void test() const {
        AppRequest req{
                .calledAppID = calledApp,
                .httpRequest = request,
                .gas = gas,
                .modifier = argennon::mocking::ascee::MockModifier(),
                .appTable = AppTable(appAccessList),
        };
        //EXPECT_CALL(req.modifier, saveVersion()).Times(1);
        Executor executor;
        auto result = executor.executeOne(&req);

        EXPECT_EQ(result.httpResponse, wantResponse);

        EXPECT_EQ(result.statusCode, wantCode);
    }
};

TEST_F(AsceeExecutorTest, ZeroGas) {
    AppLoader::global = std::make_unique<AppLoader>("testdata/single-thread/call");
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .calledApp = 11,
            .request = "test request",
            .gas = 1,
            .appAccessList = {11},
            .wantResponse = string(Executor::Error(
                    "forwarded gas is too low",
                    StatusCode::invalid_operation,
                    long_id(0)).toHttpResponse(buf)),
            .wantCode = int(StatusCode::invalid_operation)
    };
    SUB_TEST("zero gas", testCase);
}

TEST_F(AsceeExecutorTest, OneLevelCall) {
    AppLoader::global = std::make_unique<AppLoader>("testdata/single-thread/call");
    AppTestCase testCase{
            .calledApp = 11,
            .request = "test request",
            .gas = NORMAL_GAS,
            .appAccessList = {11},
            .wantResponse = "test request is DONE!",
            .wantCode = 200
    };
    SUB_TEST("one level call", testCase);
}

TEST_F(AsceeExecutorTest, TwoLevelCall) {
    AppLoader::global = std::make_unique<AppLoader>("testdata/single-thread/call");
    AppTestCase testCase{
            .calledApp = 15,
            .request = "test request",
            .gas = NORMAL_GAS,
            .appAccessList = {11, 15},
            .wantResponse = "request from 15 is DONE! got in 15",
            .wantCode = 200
    };
    SUB_TEST("two level call", testCase);
}

TEST_F(AsceeExecutorTest, AppNotFound) {
    AppLoader::global = std::make_unique<AppLoader>("testdata/single-thread/call");
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .calledApp = 16,
            .request = "test request",
            .gas = NORMAL_GAS,
            .appAccessList = {16, 555},
            .wantResponse = string(Executor::Error(
                    "app does not exist",
                    StatusCode::not_found,
                    long_id(16)).toHttpResponse(buf)) + " wrong app!",
            .wantCode = 200
    };
    SUB_TEST("", testCase);
}

TEST_F(AsceeExecutorTest, AppNotDeclared) {
    AppLoader::global = std::make_unique<AppLoader>("testdata/single-thread/call");
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .calledApp = 15,
            .request = "test request",
            .gas = NORMAL_GAS,
            .appAccessList = {15},
            .wantResponse = string(Executor::Error(
                    "app/11 was not declared in the call list",
                    StatusCode::limit_violated,
                    long_id(15)).toHttpResponse(buf)) + " got in 15",
            .wantCode = 200
    };

    SUB_TEST("not declared", testCase);
}

TEST_F(AsceeExecutorTest, SimpleTimeOut) {
    AppLoader::global = std::make_unique<AppLoader>("testdata/single-thread/timeout");
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .calledApp = 10,
            .request = "test request",
            .gas = NORMAL_GAS,
            .appAccessList = {10},
            .wantResponse = string(Executor::Error(
                    "cpu timer expired",
                    StatusCode::execution_timeout,
                    long_id(10)).toHttpResponse(buf)),
            .wantCode = 421
    };
    SUB_TEST("time out", testCase);
}

TEST_F(AsceeExecutorTest, CalledTimeOut) {
    AppLoader::global = std::make_unique<AppLoader>("testdata/single-thread/timeout");
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .calledApp = 12,
            .request = "test request",
            .gas = NORMAL_GAS,
            .appAccessList = {12, 10},
            .wantResponse = string(Executor::Error(
                    "cpu timer expired",
                    StatusCode::execution_timeout,
                    long_id(10)).toHttpResponse(buf)) + " TOO LONG...",
            .wantCode = 200
    };
    SUB_TEST("time out", testCase);
}

TEST_F(AsceeExecutorTest, SimpleStackOverflow) {
    AppLoader::global = std::make_unique<AppLoader>("testdata/single-thread/stack-overflow");
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .calledApp = 13,
            .request = "test request",
            .gas = NORMAL_GAS,
            .appAccessList = {13},
            .wantResponse = string(Executor::Error(
                    "segmentation fault (possibly stack overflow)",
                    StatusCode::memory_fault,
                    long_id(13)).toHttpResponse(buf)),
            .wantCode = (int) StatusCode::memory_fault
    };
    SUB_TEST("stack overflow", testCase);
}

TEST_F(AsceeExecutorTest, CalledStackOverflow) {
    AppLoader::global = std::make_unique<AppLoader>("testdata/single-thread/stack-overflow");
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .calledApp = 14,
            .request = "test request",
            .gas = NORMAL_GAS,
            .appAccessList = {13, 14},
            .wantResponse = string(Executor::Error(
                    "segmentation fault (possibly stack overflow)",
                    StatusCode::memory_fault,
                    long_id(13)).toHttpResponse(buf)) + " OVER FLOW... fib: 832040",
            .wantCode = 200
    };
    SUB_TEST("stack overflow", testCase);
}

TEST_F(AsceeExecutorTest, CircularCallLowGas) {
    AppLoader::global = std::make_unique<AppLoader>("testdata/single-thread/circular");
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .calledApp = 17,
            .request = "test request",
            .gas = LOW_GAS,
            .appAccessList = {17, 18},
            .wantResponse = string(Executor::Error(
                    "forwarded gas is too low",
                    StatusCode::invalid_operation,
                    long_id(18)).toHttpResponse(buf)),
            .wantCode = 200
    };
    SUB_TEST("circular", testCase);
}

TEST_F(AsceeExecutorTest, CircularCallHighGas) {
    AppLoader::global = std::make_unique<AppLoader>("testdata/single-thread/circular");
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .calledApp = 17,
            .request = "test request",
            .gas = 1000000000,
            .appAccessList = {17, 18},
            .wantResponse = string(Executor::Error(
                    "max call depth reached",
                    StatusCode::limit_exceeded,
                    long_id(17)).toHttpResponse(buf)),
            .wantCode = 200
    };
    SUB_TEST("circular", testCase);
}

TEST_F(AsceeExecutorTest, FailedCalls) {
    AppLoader::global = std::make_unique<AppLoader>("testdata/single-thread/failed-call");
    StringBuffer<1024> buf;
    Executor::Error(
            "cpu timer expired",
            StatusCode::execution_timeout,
            long_id(10)).toHttpResponse(buf);
    Executor::Error(
            "SIGFPE was caught",
            StatusCode::arithmetic_error,
            long_id(20)).toHttpResponse(buf);
    Executor::Error(
            "calling self",
            StatusCode::invalid_operation,
            long_id(19)).toHttpResponse(buf);
    Executor::Error(
            "app/11 was not declared in the call list",
            StatusCode::limit_violated,
            long_id(19)).toHttpResponse(buf);
    Executor::Error(
            "append: str is too long",
            StatusCode::out_of_range,
            long_id(21)).toHttpResponse(buf);
    Executor::Error(
            "array::at: __n (which is 6) >= _Nm (which is 6)",
            StatusCode::out_of_range,
            long_id(24)).toHttpResponse(buf);

    AppTestCase testCase{
            .calledApp = 19,
            .request = "test request",
            .gas = NORMAL_GAS,
            .appAccessList = {20, 21, 10, 19, 24},
            .wantResponse = string(buf) + " all called...",
            .wantCode = 200
    };
    SUB_TEST("failed calls", testCase);
}

TEST_F(AsceeExecutorTest, SimpleReentancy) {
    AppLoader::global = std::make_unique<AppLoader>("testdata/single-thread/reentrancy");
    auto reentrancyErr = Executor::Error(
            "reentrancy is not allowed",
            StatusCode::reentrancy_attempt,
            long_id(22));
    StringBuffer<1024> buf;
    reentrancyErr.toHttpResponse(buf);
    buf.append(" Done: 2");
    reentrancyErr.toHttpResponse(buf);
    reentrancyErr.toHttpResponse(buf);

    AppTestCase testCase{
            .calledApp = 22,
            .request = "choice: 1",
            .gas = NORMAL_GAS,
            .appAccessList = {22, 23},
            .wantResponse = string(buf) + " Done: 1",
            .wantCode = 200
    };
    SUB_TEST("", testCase);
}

TEST_F(AsceeExecutorTest, SimpleDeferredCall) {
    AppLoader::global = std::make_unique<AppLoader>("testdata/single-thread/reentrancy");
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .calledApp = 22,
            .request = "choice: 4",
            .gas = NORMAL_GAS,
            .appAccessList = {22, 23},
            .wantResponse = string(Executor::Error(
                    "deferred success!",
                    StatusCode::bad_request,
                    long_id(23)).toHttpResponse(buf)),
            .wantCode = (int) StatusCode::bad_request
    };
    SUB_TEST("deferred", testCase);
}
