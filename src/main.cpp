#include <thread>
#include "loader/AppLoader.h"

#include "Executor.h"

using namespace ascee;

int main(int argc, char const* argv[]) {
    AppLoader::init("");

    Executor e;
    Transaction tr1{1};
    Transaction tr2{2};

    std::thread t1(&Executor::startSession, &e, tr1);
    std::thread t2(&Executor::startSession, &e, tr2);

    t1.join();
    t2.join();
    return 0;
}
