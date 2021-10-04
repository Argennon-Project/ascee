
#include <csignal>
#include <heap/Heap.h>
#include <argc/functions.h>
#include <loader/AppLoader.h>
#include "Executor.h"

using namespace ascee;
using std::unique_ptr, std::string_view;

thread_local SessionInfo* Executor::session = nullptr;

void Executor::sig_handler(int sig, siginfo_t* info, void* ucontext) {
    printf("Caught signal %d\n", sig);
    // important: session is not valid when sig == SIGALRM
    if (sig != SIGALRM) {
        if (session->criticalArea) {
            siglongjmp(*session->rootEnvPointer, LOOP_DETECTED);
        }
        int ret = INTERNAL_ERROR;
        if (sig == SIGUSR1) ret = REQUEST_TIMEOUT;
        if (sig == SIGUSR2) ret = REENTRANCY_DETECTED;
        siglongjmp(*session->recentEnvPointer, ret);
    } else {
        pthread_t thread_id = *static_cast<pthread_t*>(info->si_value.sival_ptr);
        printf("Caught signal %d for app %ld\n", sig, thread_id);
        pthread_kill(thread_id, SIGUSR1);
    }
}

void Executor::initHandlers() {
    struct sigaction action{};
    action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    action.sa_sigaction = Executor::sig_handler;
    sigfillset(&action.sa_mask);
    int err = 0;
    err += sigaction(SIGALRM, &action, nullptr);
    err += sigaction(SIGFPE, &action, nullptr);
    err += sigaction(SIGSEGV, &action, nullptr);
    err += sigaction(SIGBUS, &action, nullptr);
    err += sigaction(SIGUSR1, &action, nullptr);
    err += sigaction(SIGUSR2, &action, nullptr);
    if (err != 0) {
        throw std::runtime_error("error in creating handlers");
    }
}

void* Executor::registerRecoveryStack() {
    stack_t sig_stack;

    sig_stack.ss_sp = malloc(SIGSTKSZ);
    if (sig_stack.ss_sp == nullptr) {
        throw std::runtime_error("could not allocate memory for the recovery stack");
    }
    sig_stack.ss_size = SIGSTKSZ;
    sig_stack.ss_flags = 0;
    if (sigaltstack(&sig_stack, nullptr) == -1) {
        throw std::runtime_error("sigaltstack could not register the recovery stack");
    }
    return sig_stack.ss_sp;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "LocalValueEscapesScope"

string_view Executor::startSession(const Transaction& t) {
    void* p = registerRecoveryStack();
    jmp_buf env;

    SessionInfo::CallContext newCall = {
            .appID = 0,
            .remainingExternalGas = t.gas,
    };
    SessionInfo threadSession{
            .rootEnvPointer = &env,
            .heapModifier = unique_ptr<HeapModifier>(Heap::setupSession(t.calledAppID)),
            .appTable = AppLoader::global->createAppTable(t.appAccessList),
            .currentCall = &newCall,
    };
    session = &threadSession;

    int ret, jmpRet = sigsetjmp(env, true);
    if (jmpRet == 0) {
        ret = argcrt::invoke_dispatcher(255, t.calledAppID, t.request);
    } else {
        // critical error
        printf("**critical**\n");
        ret = jmpRet;
    }

    printf("returned:%d --> %s\n", ret, threadSession.response.buffer);

    session = nullptr;
    free(p);
    return {threadSession.response.buffer, static_cast<size_t>(threadSession.response.end)};
}

#pragma clang diagnostic pop

Executor::Executor() {
    initHandlers();
}

