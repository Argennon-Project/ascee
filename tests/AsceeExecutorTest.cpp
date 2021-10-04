
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

    }

    virtual ~AsceeExecutorTest() {

    }
};

TEST_F(AsceeExecutorTest, SimpleTimeOut) {
    AppLoader::global = std::make_unique<AppLoader>("appfiles/1");

    int result = executor.startSession(ascee::Transaction{
            .calledAppID = 10,
            .gas = 2500,
            .appAccessList = {10}
    });
    EXPECT_EQ(result, REQUEST_TIMEOUT);
}
