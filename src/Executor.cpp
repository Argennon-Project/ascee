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
#include <argc/functions.h>
#include <loader/AppLoader.h>
#include "Executor.h"

using namespace ascee;
using std::unique_ptr, std::string;

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

string Executor::startSession(const Transaction& t) {
    void* recoveryStack = registerRecoveryStack();
    jmp_buf env;

    SessionInfo::CallContext newCall = {
            .appID = 0,
            .remainingExternalGas = t.gas,
    };
    SessionInfo threadSession{
            .rootEnvPointer = &env,
            .heapModifier = unique_ptr<Heap::Modifier>(heap.initSession(t.calledAppID)),
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
        session->heapModifier->restoreVersion(0);
        ret = jmpRet;
    }

    session->heapModifier->writeToHeap();

    printf("returned:%d --> %s\n", ret, threadSession.response.buffer);

    session = nullptr;
    free(recoveryStack);
    // here, string constructor makes a copy of the buffer.
    return {threadSession.response.buffer, static_cast<std::size_t>(threadSession.response.end)};
}

Executor::Executor() {
    initHandlers();
}

