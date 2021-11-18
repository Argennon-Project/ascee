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

#include "executor/Executor.h"
#include "loader/AppLoader.h"
#include "argc/types.h"

#include <gtest/gtest.h>

#define NORMAL_GAS 1500
#define LOW_GAS 100

using namespace ascee;
using namespace runtime;

class AsceeExecutorTest : public ::testing::Test {
protected:
    ascee::runtime::Executor executor;
public:
    AsceeExecutorTest() {
        AppLoader::global = std::make_unique<AppLoader>("appfiles/compiled");
    }

    virtual ~AsceeExecutorTest() {

    }
};

char* getDefaultResponse(char buf[], int statusCode) {
    sprintf(buf, "HTTP/1.1 %d %s", statusCode, "OK");
    return buf;
}

TEST_F(AsceeExecutorTest, ZeroGas) {
    auto response = executor.startSession(Transaction{
            .calledAppID = 11,
            .request = STRING("test request"),
            .gas = 1,
            .appAccessList = {11}
    });

    char buf[200];
    EXPECT_STREQ(response.data(), getDefaultResponse(buf, REQUEST_TIMEOUT));
}

TEST_F(AsceeExecutorTest, OneLevelCall) {
    auto response = executor.startSession(Transaction{
            .calledAppID = 11,
            .request = STRING("test request"),
            .gas = NORMAL_GAS,
            .appAccessList = {11}
    });

    EXPECT_STREQ(response.data(), "test request is DONE!");
}

TEST_F(AsceeExecutorTest, TwoLevelCall) {
    auto response = executor.startSession(Transaction{
            .calledAppID = 15,
            .request = STRING("test request"),
            .gas = NORMAL_GAS,
            .appAccessList = {11, 15}
    });

    EXPECT_STREQ(response.data(), "request from 15 is DONE! got in 15");
}

TEST_F(AsceeExecutorTest, AppNotFound) {
    auto response = executor.startSession(Transaction{
            .calledAppID = 16,
            .request = STRING("test request"),
            .gas = NORMAL_GAS,
            .appAccessList = {16, 555}
    });
    char buf[200];
    EXPECT_STREQ(response.data(), strcat(getDefaultResponse(buf, NOT_FOUND), " wrong app!"));
}

TEST_F(AsceeExecutorTest, AppNotDeclared) {
    auto response = executor.startSession(Transaction{
            .calledAppID = 15,
            .request = STRING("test request"),
            .gas = NORMAL_GAS,
            .appAccessList = {15}
    });

    char buf[200];
    EXPECT_STREQ(response.data(), strcat(getDefaultResponse(buf, PRECONDITION_FAILED), " got in 15"));
}

TEST_F(AsceeExecutorTest, SimpleTimeOut) {
    auto response = executor.startSession(Transaction{
            .calledAppID = 10,
            .request = STRING("test request"),
            .gas = NORMAL_GAS,
            .appAccessList = {10}
    });

    char buf[200];
    EXPECT_STREQ(response.data(), getDefaultResponse(buf, REQUEST_TIMEOUT));
}

TEST_F(AsceeExecutorTest, CalledTimeOut) {
    auto response = executor.startSession(Transaction{
            .calledAppID = 12,
            .request = STRING("test request"),
            .gas = NORMAL_GAS,
            .appAccessList = {12, 10}
    });

    char buf[200];
    EXPECT_STREQ(response.data(), strcat(getDefaultResponse(buf, REQUEST_TIMEOUT), " TOO LONG..."));
}

TEST_F(AsceeExecutorTest, SimpleStackOverflow) {
    auto response = executor.startSession(Transaction{
            .calledAppID = 13,
            .request = STRING("test request"),
            .gas = NORMAL_GAS,
            .appAccessList = {13}
    });

    char buf[200];
    EXPECT_STREQ(response.data(), getDefaultResponse(buf, INTERNAL_ERROR));
}

TEST_F(AsceeExecutorTest, CalledStackOverflow) {
    auto response = executor.startSession(Transaction{
            .calledAppID = 14,
            .request = STRING("test request"),
            .gas = NORMAL_GAS,
            .appAccessList = {13, 14}
    });

    char buf[200];
    EXPECT_STREQ(response.data(), strcat(getDefaultResponse(buf, INTERNAL_ERROR), " OVER FLOW... fib: 832040"));
}

TEST_F(AsceeExecutorTest, CircularCallLowGas) {
    auto response = executor.startSession(Transaction{
            .calledAppID = 17,
            .request = STRING("test request"),
            .gas = LOW_GAS,
            .appAccessList = {17, 18}
    });

    char buf[200];
    EXPECT_STREQ(response.data(), getDefaultResponse(buf, REQUEST_TIMEOUT));
}

TEST_F(AsceeExecutorTest, CircularCallHighGas) {
    auto response = executor.startSession(Transaction{
            .calledAppID = 17,
            .request = STRING("test request"),
            .gas = 10000000000,
            .appAccessList = {17, 18}
    });

    char buf[200];
    EXPECT_STREQ(response.data(), getDefaultResponse(buf, MAX_CALL_DEPTH_REACHED));
}

TEST_F(AsceeExecutorTest, FailedCalls) {
    auto response = executor.startSession(Transaction{
            .calledAppID = 19,
            .request = STRING("test request"),
            .gas = NORMAL_GAS,
            .appAccessList = {20, 21, 10, 19}
    });

    char expected[1024], buf[200];
    getDefaultResponse(expected, REQUEST_TIMEOUT);
    strcat(expected, getDefaultResponse(buf, INTERNAL_ERROR));
    strcat(expected, getDefaultResponse(buf, NOT_FOUND));
    strcat(expected, getDefaultResponse(buf, PRECONDITION_FAILED));
    strcat(expected, getDefaultResponse(buf, INTERNAL_ERROR));
    strcat(expected, " all called...");
    EXPECT_STREQ(response.data(), expected);
}

TEST_F(AsceeExecutorTest, SimpleReentancy) {
    auto response = executor.startSession(Transaction{
            .calledAppID = 22,
            .request = STRING("choice: 1"),
            .gas = NORMAL_GAS,
            .appAccessList = {22, 23}
    });

    char buf[200];
    sprintf(buf, "%d %d %d %d done: 1", REENTRANCY_DETECTED + 1, HTTP_OK + 1, REENTRANCY_DETECTED + 1,
            REENTRANCY_DETECTED + 1);
    EXPECT_STREQ(response.data(), buf);
}

TEST_F(AsceeExecutorTest, SimpleDeferredCall) {
    auto response = executor.startSession(Transaction{
            .calledAppID = 22,
            .request = STRING("choice: 4"),
            .gas = NORMAL_GAS,
            .appAccessList = {22, 23}
    });

    char buf[200];
    EXPECT_STREQ(response.data(), getDefaultResponse(buf, FAILED_DEPENDENCY));
}
