
#include "Executor.h"
#include "loader/AppLoader.h"
#include "argc/types.h"

#include <gtest/gtest.h>


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
    ascee::AppLoader::init("appfiles/1");
    int result = executor.startSession(ascee::Transaction{10, 5000});
    EXPECT_EQ(result, REQUEST_TIMEOUT);
}
