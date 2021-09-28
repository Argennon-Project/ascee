#include <thread>
#include "loader/AppLoader.h"
#include <unordered_map>
#include <csignal>

#include "argc/runtime/session.h"

using std::thread;

void executeSession(int transactionInfo);

static void sig_handler(int sig, siginfo_t* info, void* ucontext) {
    printf("Caught signal %d\n", sig);
    if (sig != SIGALRM) {
        int* ret = (int*) malloc(sizeof(int));
        *ret = INTERNAL_ERROR;
        if (sig == SIGUSR1) *ret = REQUEST_TIMEOUT;
        pthread_exit(ret);
    } else {
        pthread_t thread_id = *static_cast<pthread_t*>(info->si_value.sival_ptr);
        printf("Caught signal %d for app %ld\n", sig, thread_id);
        pthread_kill(thread_id, SIGUSR1);
    }
}

void init_handlers() {
    struct sigaction action{};

    action.sa_flags = SA_SIGINFO;
    action.sa_sigaction = sig_handler;
    sigfillset(&action.sa_mask);
    int err = 0;
    err += sigaction(SIGALRM, &action, nullptr);
    err += sigaction(SIGFPE, &action, nullptr);
    err += sigaction(SIGUSR1, &action, nullptr);
    err += sigaction(SIGSEGV, &action, nullptr);
    err += sigaction(SIGBUS, &action, nullptr);
    if (err != 0) {
        printf("error in creating handlers\n");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char const* argv[]) {
    ascee::AppLoader::init(1);
    ascee::AppLoader::init(2);
    init_handlers();

    thread t1(executeSession, 111);
    //thread t2(executeSession, 222);

    t1.join();
    //t2.join();
    return 0;
}
