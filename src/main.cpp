#include <thread>
#include "loader/AppLoader.h"

#include "Executor.h"

using namespace ascee;

int main(int argc, char const* argv[]) {
    AppLoader::global = std::make_unique<AppLoader>("");

    Executor e;
    Transaction tr1{1, "req", 4, 1000000000000, {1, 2, 3}};
    /// Transaction tr2{2, "req",5000, {2}};

    std::thread t1(&Executor::startSession, &e, tr1);
    //std::thread t2(&Executor::startSession, &e, tr2);

    t1.join();
    //t2.join();
    return 0;
}
