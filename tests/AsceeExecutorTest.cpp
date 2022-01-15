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
    Executor executor;
public:
    AsceeExecutorTest() {
        AppLoader::global = std::make_unique<AppLoader>("appFiles/compiled");
    }
};

#include <gmock/gmock.h>

class MockModifier {
public:
    MOCK_METHOD(void, restoreVersion, (int16_t version));
    MOCK_METHOD(int16_t, saveVersion, ());
};

struct AppTestCase {
    Executor* executor;
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
                .appTable = AppTable(appAccessList)
        };
        auto result = executor->executeOne(&req);

        EXPECT_EQ(result.httpResponse, wantResponse);

        EXPECT_EQ(result.statusCode, wantCode);
    }
};

TEST_F(AsceeExecutorTest, ZeroGas) {
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .executor = &executor,
            .calledApp = 11,
            .request = "test request",
            .gas = 1,
            .appAccessList = {11},
            .wantResponse = string(Executor::GenericError(
                    "forwarded gas is too low",
                    StatusCode::invalid_operation,
                    long_id(0)).toHttpResponse(buf)),
            .wantCode = int(StatusCode::invalid_operation)
    };
    SUB_TEST("zero gas", testCase);
}

TEST_F(AsceeExecutorTest, OneLevelCall) {
    AppTestCase testCase{
            .executor = &executor,
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
    AppTestCase testCase{
            .executor = &executor,
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
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .executor = &executor,
            .calledApp = 16,
            .request = "test request",
            .gas = NORMAL_GAS,
            .appAccessList = {16, 555},
            .wantResponse = string(Executor::GenericError(
                    "app does not exist",
                    StatusCode::not_found,
                    long_id(16)).toHttpResponse(buf)) + " wrong app!",
            .wantCode = 200
    };
    SUB_TEST("", testCase);
}

TEST_F(AsceeExecutorTest, AppNotDeclared) {
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .executor = &executor,
            .calledApp = 15,
            .request = "test request",
            .gas = NORMAL_GAS,
            .appAccessList = {15},
            .wantResponse = string(Executor::GenericError(
                    "app/11 was not declared in the call list",
                    StatusCode::limit_violated,
                    long_id(15)).toHttpResponse(buf)) + " got in 15",
            .wantCode = 200
    };
    SUB_TEST("not declared", testCase);
}

TEST_F(AsceeExecutorTest, SimpleTimeOut) {
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .executor = &executor,
            .calledApp = 10,
            .request = "test request",
            .gas = NORMAL_GAS,
            .appAccessList = {10},
            .wantResponse = string(Executor::GenericError(
                    "cpu timer expired",
                    StatusCode::execution_timeout,
                    long_id(10)).toHttpResponse(buf)),
            .wantCode = 421
    };
    SUB_TEST("time out", testCase);
}

TEST_F(AsceeExecutorTest, CalledTimeOut) {
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .executor = &executor,
            .calledApp = 12,
            .request = "test request",
            .gas = NORMAL_GAS,
            .appAccessList = {12, 10},
            .wantResponse = string(Executor::GenericError(
                    "cpu timer expired",
                    StatusCode::execution_timeout,
                    long_id(10)).toHttpResponse(buf)) + " TOO LONG...",
            .wantCode = 200
    };
    SUB_TEST("time out", testCase);
}

TEST_F(AsceeExecutorTest, SimpleStackOverflow) {
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .executor = &executor,
            .calledApp = 13,
            .request = "test request",
            .gas = NORMAL_GAS,
            .appAccessList = {13},
            .wantResponse = string(Executor::GenericError(
                    "segmentation fault (possibly stack overflow)",
                    StatusCode::internal_error,
                    long_id(13)).toHttpResponse(buf)),
            .wantCode = 500
    };
    SUB_TEST("stack overflow", testCase);
}

TEST_F(AsceeExecutorTest, CalledStackOverflow) {
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .executor = &executor,
            .calledApp = 14,
            .request = "test request",
            .gas = NORMAL_GAS,
            .appAccessList = {13, 14},
            .wantResponse = string(Executor::GenericError(
                    "segmentation fault (possibly stack overflow)",
                    StatusCode::internal_error,
                    long_id(13)).toHttpResponse(buf)) + " OVER FLOW... fib: 832040",
            .wantCode = 200
    };
    SUB_TEST("stack overflow", testCase);
}

TEST_F(AsceeExecutorTest, CircularCallLowGas) {
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .executor = &executor,
            .calledApp = 17,
            .request = "test request",
            .gas = LOW_GAS,
            .appAccessList = {17, 18},
            .wantResponse = string(Executor::GenericError(
                    "forwarded gas is too low",
                    StatusCode::invalid_operation,
                    long_id(18)).toHttpResponse(buf)),
            .wantCode = 200
    };
    SUB_TEST("circular", testCase);
}

TEST_F(AsceeExecutorTest, CircularCallHighGas) {
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .executor = &executor,
            .calledApp = 17,
            .request = "test request",
            .gas = 1000000000,
            .appAccessList = {17, 18},
            .wantResponse = string(Executor::GenericError(
                    "max call depth reached",
                    StatusCode::limit_exceeded,
                    long_id(17)).toHttpResponse(buf)),
            .wantCode = 200
    };
    SUB_TEST("circular", testCase);
}

TEST_F(AsceeExecutorTest, FailedCalls) {
    StringBuffer<1024> buf;
    Executor::GenericError(
            "cpu timer expired",
            StatusCode::execution_timeout,
            long_id(10)).toHttpResponse(buf);
    Executor::GenericError(
            "SIGFPE was caught",
            StatusCode::arithmetic_error,
            long_id(20)).toHttpResponse(buf);
    Executor::GenericError(
            "calling self",
            StatusCode::invalid_operation,
            long_id(19)).toHttpResponse(buf);
    Executor::GenericError(
            "app/11 was not declared in the call list",
            StatusCode::limit_violated,
            long_id(19)).toHttpResponse(buf);
    Executor::GenericError(
            "append: str is too long",
            StatusCode::internal_error,
            long_id(19)).toHttpResponse(buf);
    // caller can see out_of_range exception so server is 19

    AppTestCase testCase{
            .executor = &executor,
            .calledApp = 19,
            .request = "test request",
            .gas = NORMAL_GAS,
            .appAccessList = {20, 21, 10, 19},
            .wantResponse = string(buf) + " all called...",
            .wantCode = 200
    };
    SUB_TEST("failed calls", testCase);
}

TEST_F(AsceeExecutorTest, SimpleReentancy) {
    auto reentrancyErr = Executor::GenericError(
            "reentrancy is not allowed",
            StatusCode::reentrancy_attempt,
            long_id(22));
    StringBuffer<1024> buf;
    reentrancyErr.toHttpResponse(buf);
    buf.append(" Done: 2");
    reentrancyErr.toHttpResponse(buf);
    reentrancyErr.toHttpResponse(buf);

    AppTestCase testCase{
            .executor = &executor,
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
    StringBuffer<1024> buf;
    AppTestCase testCase{
            .executor = &executor,
            .calledApp = 22,
            .request = "choice: 4",
            .gas = NORMAL_GAS,
            .appAccessList = {22, 23},
            .wantResponse = string(Executor::GenericError(
                    "deferred success!",
                    StatusCode::internal_error,
                    long_id(23)).toHttpResponse(buf)),
            .wantCode = 500
    };
    SUB_TEST("deferred", testCase);
}
