#include <thread>
#include "loader/AppLoader.h"
#include <unordered_map>
#include <csignal>

using std::thread;

void executeSession(int transactionInfo);

static void sig_handler(int sig, siginfo_t* info, void* ucontext) {
    if (sig == SIGFPE || sig == SIGABRT || sig == SIGSEGV) {
        int* ret = (int*) malloc(sizeof(int));
        *ret = INTERNAL_ERROR;
        if (sig == SIGABRT)
            *ret = LOOP_DETECTED;
        pthread_exit(ret);
    }
    else if (sig == SIGALRM)
    {
        printf("Caught signal %d for %p\n", sig, info->si_value.sival_ptr);
        pthread_kill(*(pthread_t *)info->si_value.sival_ptr, SIGABRT);
    }
}

void init_handlers()
{
    struct sigaction action{};

    action.sa_flags = SA_SIGINFO;
    action.sa_sigaction = sig_handler;
    sigemptyset(&action.sa_mask);
    int err = 0;
    err += sigaction(SIGALRM, &action, NULL);
    err += sigaction(SIGABRT, &action, NULL);
    err += sigaction(SIGFPE, &action, NULL);
    err += sigaction(SIGSEGV, &action, NULL);
    if (err != 0)
    {
        printf("error in creating handlers\n");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char const* argv[]) {
    ascee::AppLoader::init();
    init_handlers();

    thread t1(executeSession, 111);
    thread t2(executeSession, 222);


    t1.join();
    t2.join();
    return 0;
}
