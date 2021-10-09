#include <thread>
#include "loader/AppLoader.h"

#include "Executor.h"

using namespace ascee;

int main(int argc, char const* argv[]) {
    HeapModifier hp(2);
    argc::short_id_t chunk = 111;
    hp.changeContext(2);
    hp.loadChunk(chunk);
    hp.store(5, 0x1122334455667788);
    printf("\nread->%lx\n", hp.load<int64_t>(5));

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
