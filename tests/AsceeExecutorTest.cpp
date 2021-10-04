
#include "Executor.h"
#include "loader/AppLoader.h"
#include "argc/types.h"

#include <gtest/gtest.h>

using ascee::AppLoader;

class AsceeExecutorTest : public ::testing::Test {
protected:
    ascee::Executor executor;
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
    auto response = executor.startSession(ascee::Transaction{
            .calledAppID = 11,
            .request = STRING("test request"),
            .gas = 1,
            .appAccessList = {11}
    });

    char buf[200];
    EXPECT_STREQ(response.data(), getDefaultResponse(buf, REQUEST_TIMEOUT));
}

TEST_F(AsceeExecutorTest, OneLevelCall) {
    auto response = executor.startSession(ascee::Transaction{
            .calledAppID = 11,
            .request = STRING("test request"),
            .gas = 500,
            .appAccessList = {11}
    });

    EXPECT_STREQ(response.data(), "test request is DONE!");
}

TEST_F(AsceeExecutorTest, TwoLevelCall) {
    auto response = executor.startSession(ascee::Transaction{
            .calledAppID = 15,
            .request = STRING("test request"),
            .gas = 500,
            .appAccessList = {11, 15}
    });

    EXPECT_STREQ(response.data(), "request from 15 is DONE! got in 15");
}

TEST_F(AsceeExecutorTest, AppNotFound) {
    auto response = executor.startSession(ascee::Transaction{
            .calledAppID = 16,
            .request = STRING("test request"),
            .gas = 500,
            .appAccessList = {16, 555}
    });

    char buf[200];
    EXPECT_STREQ(response.data(), strcat(getDefaultResponse(buf, NOT_FOUND), " wrong app!"));
}

TEST_F(AsceeExecutorTest, AppNotDeclared) {
    auto response = executor.startSession(ascee::Transaction{
            .calledAppID = 15,
            .request = STRING("test request"),
            .gas = 500,
            .appAccessList = {15}
    });

    char buf[200];
    EXPECT_STREQ(response.data(), strcat(getDefaultResponse(buf, PRECONDITION_FAILED), " got in 15"));
}

TEST_F(AsceeExecutorTest, SimpleTimeOut) {
    auto response = executor.startSession(ascee::Transaction{
            .calledAppID = 10,
            .request = STRING("test request"),
            .gas = 1500,
            .appAccessList = {10}
    });

    char buf[200];
    EXPECT_STREQ(response.data(), getDefaultResponse(buf, REQUEST_TIMEOUT));
}

TEST_F(AsceeExecutorTest, CalledTimeOut) {
    auto response = executor.startSession(ascee::Transaction{
            .calledAppID = 12,
            .request = STRING("test request"),
            .gas = 1500,
            .appAccessList = {12, 10}
    });

    char buf[200];
    EXPECT_STREQ(response.data(), strcat(getDefaultResponse(buf, REQUEST_TIMEOUT), " TOO LONG..."));
}

TEST_F(AsceeExecutorTest, SimpleStackOverflow) {
    auto response = executor.startSession(ascee::Transaction{
            .calledAppID = 13,
            .request = STRING("test request"),
            .gas = 1500,
            .appAccessList = {13}
    });

    char buf[200];
    EXPECT_STREQ(response.data(), getDefaultResponse(buf, INTERNAL_ERROR));
}

TEST_F(AsceeExecutorTest, CalledStackOverflow) {
    auto response = executor.startSession(ascee::Transaction{
            .calledAppID = 14,
            .request = STRING("test request"),
            .gas = 1500,
            .appAccessList = {13, 14}
    });

    char buf[200];
    EXPECT_STREQ(response.data(), strcat(getDefaultResponse(buf, INTERNAL_ERROR), " OVER FLOW..."));
}

TEST_F(AsceeExecutorTest, CircularCallLowGas) {
    auto response = executor.startSession(ascee::Transaction{
            .calledAppID = 17,
            .request = STRING("test request"),
            .gas = 100,
            .appAccessList = {17, 18}
    });

    char buf[200];
    EXPECT_STREQ(response.data(), getDefaultResponse(buf, REQUEST_TIMEOUT));
}

TEST_F(AsceeExecutorTest, CircularCallHighGas) {
    auto response = executor.startSession(ascee::Transaction{
            .calledAppID = 17,
            .request = STRING("test request"),
            .gas = 10000000000,
            .appAccessList = {17, 18}
    });

    char buf[200];
    EXPECT_STREQ(response.data(), getDefaultResponse(buf, INTERNAL_ERROR));
}
