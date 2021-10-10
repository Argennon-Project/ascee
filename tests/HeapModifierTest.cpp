
#include <gtest/gtest.h>
#include <argc/types.h>

#define private public

#include "heap/Heap.h"

using namespace ascee;

class HeapModifierTest : public ::testing::Test {
protected:
    ascee::byte tempHeap[256] = {2, 2, 2, 1, 2, 2, 2, 1};
    ascee::Heap::Modifier modifier;
public:
    HeapModifierTest() {
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
    }

    void memDump() {
        for (unsigned char i: tempHeap) {
            printf("%d ", i);
        }
        printf("\n");
    }
};


TEST_F(HeapModifierTest, SimpleReadWrite) {
    modifier.loadContext(1);
    modifier.loadChunk(short_id_t(10));
    auto got = modifier.load<int64>(100);
    EXPECT_EQ(got, 0x102020201020202) << "got: 0x" << std::hex << got;

    modifier.loadChunk(std_id_t(10));
    modifier.store(100, 123456789);
    got = modifier.load<int64>(100);
    EXPECT_EQ(got, 123456789);

    modifier.loadChunk(short_id_t(10));
    EXPECT_THROW(modifier.store(100, 444444), std::out_of_range);

    int128 big = 1;
    big <<= 100;
    modifier.store(108, big);
    EXPECT_EQ(tempHeap[20], 16);

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

    memDump();
}

TEST_F(HeapModifierTest, SimpleVersioning) {
    printf("%d\n", tempHeap[0]);
}