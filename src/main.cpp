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

void registerRecoveryStack() {
    stack_t ss;

    ss.ss_sp = malloc(SIGSTKSZ);
    if (ss.ss_sp == nullptr) {
        throw std::runtime_error("could not allocate memory for the recovery stack");
    }
    ss.ss_size = SIGSTKSZ;
    ss.ss_flags = 0;
    if (sigaltstack(&ss, nullptr) == -1) {
        throw std::runtime_error("sigaltstack could not register the recovery stack");
    }
}

void init_handlers() {
    struct sigaction action{};
    action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    action.sa_sigaction = sig_handler;
    sigfillset(&action.sa_mask);
    int err = 0;
    //err += sigaction(SIGALRM, &action, nullptr);
    //err += sigaction(SIGFPE, &action, nullptr);
    // err += sigaction(SIGUSR1, &action, nullptr);
    err += sigaction(SIGSEGV, &action, nullptr);
    // err += sigaction(SIGBUS, &action, nullptr);
    if (err != 0) {
        throw std::runtime_error("error in creating handlers");
    }
    printf("herrrreee!\n");
}

int foo() {
    return foo();
}

void thtest() {
    registerRecoveryStack();
    foo();
}

int main(int argc, char const* argv[]) {
    //ascee::AppLoader::init(1);
    //ascee::AppLoader::init(2);
    init_handlers();

    std::thread t(thtest);
    t.join();

    //thread t1(executeSession, 111);
    //thread t2(executeSession, 222);

    //t1.join();
    //t2.join();
    return 0;
}
