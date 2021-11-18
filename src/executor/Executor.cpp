// Copyright (c) 2021 aybehrouz <behrouz_ayati@yahoo.com>. All rights reserved.
// This file is part of the C++ implementation of the Argennon smart contract
// Execution Environment (AscEE).
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option) any
// later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License
// for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

#include <csignal>
#include <heap/Heap.h>
#include <loader/AppLoader.h>
#include <argc/functions.h>
#include "Executor.h"

using namespace ascee;
using namespace ascee::runtime;
using std::unique_ptr, std::string, std::to_string;

int invoke_dispatcher();

thread_local SessionInfo* Executor::session = nullptr;

void Executor::sig_handler(int sig, siginfo_t* info, void* ucontext) {
    printf("Caught signal %d\n", sig);
    // important: session is not valid when sig == SIGALRM
    if (sig != SIGALRM) {
        if (session->criticalArea) {
            throw execution_error(LOOP_DETECTED);
        }
        int ret = INTERNAL_ERROR;
        if (sig == SIGUSR1) ret = REQUEST_TIMEOUT;
        if (sig == SIGUSR2) ret = REENTRANCY_DETECTED;
        throw execution_error(ret);
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
        throw std::runtime_error("error in registering the recovery stack");
    }
    return sig_stack.ss_sp;
}

string Executor::startSession(const Transaction& t) {
    void* recoveryStack = registerRecoveryStack();

    SessionInfo::CallContext newCall = {
            .appID = 0,
            .remainingExternalGas = t.gas,
    };
    SessionInfo threadSession{
            .heapModifier = unique_ptr<Heap::Modifier>(heap.initSession(t.calledAppID)),
            .appTable = AppLoader::global->createAppTable(t.appAccessList),
            .currentCall = &newCall,
    };
    session = &threadSession;

    int ret;
    try {
        ret = argc::invoke_dispatcher(255, t.calledAppID, t.request);
    } catch (const execution_error& e) {
        // critical error
        printf("**critical**\n");
        session->heapModifier->restoreVersion(0);
        ret = e.statusCode();
    }

    session->heapModifier->writeToHeap();

    session = nullptr;
    free(recoveryStack);
    // here, string constructor makes a copy of the buffer.
    return string(string_t(threadSession.response));
}

Executor::Executor() {
    initHandlers();
}

struct InvocationArgs {
    int (* invoker)(byte, std_id_t, string_t);

    byte forwarded_gas;
    std_id_t app_id;
    string_t request;
    SessionInfo* session;
};

static
void* threadStart(void* voidArgs) {
    auto* args = (InvocationArgs*) voidArgs;

    void* recoveryStack = Executor::registerRecoveryStack();
    Executor::session = args->session;

    int* ret = (int*) malloc(sizeof(int));
    *ret = args->invoker(args->forwarded_gas, args->app_id, args->request);

    free(recoveryStack);
    return ret;
}

int Executor::controlledExec(int (* invoker)(byte, std_id_t, string_t),
                             byte forwarded_gas, std_id_t app_id, string_t request, size_t stackSize) {
    pthread_t threadID;
    pthread_attr_t threadAttr;

    int err = pthread_attr_init(&threadAttr);
    if (err) throw std::runtime_error(to_string(errno) + ": failed to init thread attributes");

    err = pthread_attr_setstacksize(&threadAttr, stackSize);
    if (err) throw std::runtime_error(to_string(errno) + ": can't set stack size");

    InvocationArgs args = {
            .invoker = invoker,
            .forwarded_gas = forwarded_gas,
            .app_id = app_id,
            .request = request,
            .session = Executor::getSession()
    };

    err = pthread_create(&threadID, &threadAttr, threadStart, &args);
    if (err) throw std::runtime_error(to_string(errno) + ": thread creation failed");

    pthread_attr_destroy(&threadAttr);
    // We ignore errors here

    void* retPtr;
    err = pthread_join(threadID, &retPtr);
    if (err) throw std::runtime_error(to_string(errno) + ": pthread_join failed");

    int ret = *(int*) retPtr;
    free(retPtr);
    return ret;
}

