#include <thread>
#include "loader/AppLoader.h"

#include "Executor.h"

using namespace ascee;

int main(int argc, char const* argv[]) {
    Heap heap;
    auto* modifier = heap.initSession(2);
    short_id_t chunk = 111;
    modifier->loadContext(2);
    modifier->loadChunk(chunk);
    modifier->saveVersion();
    modifier->store(5, 0x1122334455667788);
    printf("\nread->%lx\n", modifier->load<int64_t>(5));

    modifier->restoreVersion(0);
    int64 xyz = modifier->load<int64_t>(5);
    printf("\nread->%lx\n", xyz);

    // delete modifier;

    uint8_t test[256];
    int64_t x = 0x12345678;
    auto* t = (int64_t*) (test);
    t[0] = x;
    for (int i = 0; i < 15; ++i) {
        printf("%x ", test[i]);
    }
    printf("\n%lx", t[0]);

    /* AppLoader::global = std::make_unique<AppLoader>("");

     Executor e;
     Transaction tr1{1, "req", 4, 1000000000000, {1, 2, 3}};
     /// Transaction tr2{2, "req",5000, {2}};

     std::thread t1(&Executor::startSession, &e, tr1);
     //std::thread t2(&Executor::startSession, &e, tr2);

     t1.join();
     //t2.join();
     return 0;*/
}
