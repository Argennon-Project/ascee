
#include <gtest/gtest.h>
#include <argc/types.h>

#define private public

#include "heap/Heap.h"

using namespace ascee;

class HeapModifierDeathTest : public ::testing::Test {
protected:
    ascee::byte tempHeap[256] = {2, 2, 2, 1, 2, 2, 2, 1};
    ascee::Heap::Modifier modifier;
public:
    HeapModifierDeathTest() {
        // appID = 1
        modifier.defineAccessBlock(ascee::Heap::Pointer(tempHeap),
                                   1, short_id_t(10), 100,
                                   8, false);
        modifier.defineAccessBlock(ascee::Heap::Pointer(tempHeap + 8),
                                   1, short_id_t(10), 108,
                                   16, true);
        modifier.defineAccessBlock(ascee::Heap::Pointer(tempHeap + 50),
                                   1, short_id_t(11), 100,
                                   8, true);
        modifier.defineAccessBlock(ascee::Heap::Pointer(tempHeap + 58),
                                   1, short_id_t(11), 120,
                                   8, true);
        modifier.defineAccessBlock(ascee::Heap::Pointer(tempHeap + 100),
                                   1, std_id_t(10), 100,
                                   8, true);
        // appID = 2
        modifier.defineAccessBlock(ascee::Heap::Pointer(tempHeap + 150),
                                   2, short_id_t(10), 100,
                                   8, true);
        modifier.defineAccessBlock(ascee::Heap::Pointer(tempHeap + 158),
                                   2, short_id_t(11), 100,
                                   8, true);

        *(int64*) (tempHeap + 50) = 789;
        *(int64*) (tempHeap + 58) = 321;
    }

    void memDump() {
        for (unsigned char i: tempHeap) {
            printf("%d ", i);
        }
        printf("\n");
    }
};

TEST_F(HeapModifierDeathTest, SimpleReadWrite) {
    modifier.saveVersion();
    modifier.loadContext(1);
    modifier.loadChunk(short_id_t(10));
    auto got = modifier.load<int64>(100);
    EXPECT_EQ(got, 0x102020201020202) << "got: 0x" << std::hex << got;

    modifier.loadChunk(std_id_t(10));
    modifier.store<int64>(100, 123456789);
    got = modifier.load<int64>(100);
    EXPECT_EQ(got, 123456789);

    modifier.loadChunk(short_id_t(10));
    EXPECT_THROW(modifier.store(100, 444444), std::out_of_range);

    int128 big = 1;
    big <<= 100;
    modifier.store(108, big);
    auto gotBig = modifier.load<int128>(108);
    EXPECT_TRUE(big == gotBig);

    modifier.loadContext(2);

    EXPECT_EXIT(modifier.store(0, 0), testing::KilledBySignal(SIGSEGV), "");

    modifier.loadChunk(short_id_t(10));
    EXPECT_THROW(modifier.store(100, big), std::out_of_range);

    EXPECT_THROW(modifier.load<int128>(100), std::out_of_range);

    modifier.store(100, 444555666777888);
    got = modifier.load<int64>(100);
    EXPECT_EQ(got, 444555666777888);

    modifier.loadContext(1);
    modifier.loadChunk(std_id_t(10));
    got = modifier.load<int64>(100);
    EXPECT_EQ(got, 123456789);

    modifier.writeToHeap();

    EXPECT_EQ(tempHeap[3], 1);
    EXPECT_EQ(tempHeap[20], 16);
    EXPECT_EQ(tempHeap[150], 0x20);
    EXPECT_EQ(tempHeap[100], 0x15);

    memDump();
}

TEST_F(HeapModifierDeathTest, VersionZero) {
    modifier.loadContext(1);
    modifier.loadChunk(short_id_t(10));
    auto got = modifier.load<int64>(100);
    EXPECT_EQ(got, 0x102020201020202) << "got: 0x" << std::hex << got;

    EXPECT_THROW(modifier.restoreVersion(1), std::runtime_error);
}


TEST_F(HeapModifierDeathTest, SimpleVersioning) {
    modifier.loadContext(1);
    modifier.loadChunk(short_id_t(11));

    auto v0 = modifier.saveVersion();
    modifier.store<int64>(100, 1);
    modifier.store<int64>(120, 11);

    auto v1 = modifier.saveVersion();
    modifier.store<int64>(100, 2);

    auto v2 = modifier.saveVersion();
    modifier.store<int64>(120, 22);

    auto v3 = modifier.saveVersion();
    modifier.store<int64>(100, 3);
    modifier.store<int64>(120, 33);

    EXPECT_EQ(modifier.load<int64>(100), 3);
    EXPECT_EQ(modifier.load<int64>(120), 33);

    modifier.restoreVersion(v3);
    EXPECT_EQ(modifier.load<int64>(100), 2);
    EXPECT_EQ(modifier.load<int64>(120), 22);

    modifier.restoreVersion(v2);
    EXPECT_EQ(modifier.load<int64>(100), 2);
    EXPECT_EQ(modifier.load<int64>(120), 11);

    EXPECT_THROW(modifier.restoreVersion(v3), std::runtime_error);

    modifier.restoreVersion(v1);
    EXPECT_EQ(modifier.load<int64>(100), 1);
    EXPECT_EQ(modifier.load<int64>(120), 11);

    modifier.restoreVersion(v0);
    EXPECT_EQ(modifier.load<int64>(100), 789);
    EXPECT_EQ(modifier.load<int64>(120), 321);
}

TEST_F(HeapModifierDeathTest, RestoringMultiVersions) {
    modifier.loadContext(1);
    modifier.loadChunk(short_id_t(11));

    modifier.saveVersion();
    modifier.store<int64>(100, 1);
    modifier.store<int64>(120, 11);

    auto v1 = modifier.saveVersion();
    modifier.store<int64>(100, 2);

    modifier.saveVersion();
    modifier.store<int64>(120, 22);

    modifier.saveVersion();
    modifier.store<int64>(100, 3);
    modifier.store<int64>(120, 33);

    EXPECT_EQ(modifier.load<int64>(100), 3);
    EXPECT_EQ(modifier.load<int64>(120), 33);

    modifier.restoreVersion(v1);
    EXPECT_EQ(modifier.load<int64>(100), 1);
    EXPECT_EQ(modifier.load<int64>(120), 11);

    modifier.store<int64>(120, 44);
    auto v4 = modifier.saveVersion();

    modifier.store<int64>(100, 5);
    modifier.store<int64>(120, 55);

    modifier.saveVersion();
    modifier.store<int64>(100, 6);

    modifier.restoreVersion(v4);
    EXPECT_EQ(modifier.load<int64>(100), 1);
    EXPECT_EQ(modifier.load<int64>(120), 44);

    modifier.restoreVersion(0);
    EXPECT_EQ(modifier.load<int64>(100), 789);
    EXPECT_EQ(modifier.load<int64>(120), 321);
}