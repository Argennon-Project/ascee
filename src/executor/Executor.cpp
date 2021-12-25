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
#include "heap/PageCache.h"
#include <loader/AppLoader.h>
#include <argc/functions.h>
#include <thread>
#include "Executor.h"

using namespace ascee;
using namespace ascee::runtime;
using std::unique_ptr, std::string, std::to_string;

thread_local Executor::SessionInfo* Executor::session = nullptr;

void Executor::sig_handler(int sig, siginfo_t* info, void*) {
    printf("Caught signal %d\n", sig);
    // important: session is not valid when sig == SIGALRM
    if (sig != SIGALRM) {
        if (session->criticalArea) {
            throw std::runtime_error("signal raised from critical area");
        }
        if (sig == SIGSEGV) throw GenericError("segmentation fault (possibly stack overflow)");
        if (sig == SIGUSR1) throw GenericError("cpu timer expired", StatusCode::execution_timeout);
        if (sig == SIGFPE) throw GenericError("SIGFPE was caught", StatusCode::arithmetic_error);
        else throw GenericError("a signal was caught");
    } else {
        pthread_t thread_id = *static_cast<pthread_t*>(info->si_value.sival_ptr);
        printf("Caught signal %d for app %ld\n", sig, thread_id);
        pthread_kill(thread_id, SIGUSR1);
    }
}

static inline
void maskSignals(int how) {
    sigset_t set;

// Block timer's signal
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGKILL);
    int s = pthread_sigmask(how, &set, nullptr);
    if (s != 0) throw std::runtime_error("error in masking signals");

}

void Executor::blockSignals() {
    maskSignals(SIG_BLOCK);
    Executor::getSession()->criticalArea = true;
}

void Executor::unBlockSignals() {
    maskSignals(SIG_UNBLOCK);
    Executor::getSession()->criticalArea = false;
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
    if (err) throw std::runtime_error("error in creating handlers");
}

void* Executor::registerRecoveryStack() {
    stack_t sig_stack;

    sig_stack.ss_sp = malloc(SIGSTKSZ);
    if (sig_stack.ss_sp == nullptr) throw std::runtime_error("could not allocate memory for the recovery stack");

    sig_stack.ss_size = SIGSTKSZ;
    sig_stack.ss_flags = 0;

    int err = sigaltstack(&sig_stack, nullptr);
    if (err) throw std::runtime_error("error in registering the recovery stack");

    return sig_stack.ss_sp;
}

AppResponse Executor::executeOne(AppRequest* tx) {
    AppResponse result{.txID = tx->id};
    session = nullptr;
    try {
        SessionInfo threadSession{
                .request = tx,
        };
        session = &threadSession;

        CallResourceContext rootResourceCtx(tx->gas);
        CallInfoContext rootInfoCtx;
        result.statusCode = argc::invoke_dispatcher(255, tx->calledAppID, string_c(tx->httpRequest));
        // here, string's assignment operator makes a copy of the buffer.
        result.response = StringView(session->response);
        rootResourceCtx.complete();
        session->heapModifier.writeToHeap();
    } catch (const std::out_of_range& err) {

    }
    session = nullptr;

    return result;
}

Executor::Executor() {
    initHandlers();
}

struct InvocationArgs {
    int (* invoker)(long_id, string_c);

    long_id app_id;
    string_c request;
    int_fast64_t execTime;
    Executor::SessionInfo* session;
};

void* Executor::threadStart(void* voidArgs) {
    auto* args = (InvocationArgs*) voidArgs;

    void* recoveryStack = Executor::registerRecoveryStack();
    session = args->session;

    int* ret = (int*) malloc(sizeof(int));
    if (ret == nullptr) throw std::runtime_error("memory allocation error");

    ThreadCpuTimer cpuTimer;
    cpuTimer.setAlarm(args->execTime);

    *ret = args->invoker(args->app_id, args->request);

    free(recoveryStack);
    return ret;
}

int Executor::controlledExec(int (* invoker)(long_id, string_c),
                             long_id app_id, string_c request,
                             int_fast64_t execTime, size_t stackSize) {
    pthread_t threadID;
    pthread_attr_t threadAttr;

    int err = pthread_attr_init(&threadAttr);
    if (err) throw std::runtime_error(to_string(errno) + ": failed to init thread attributes");

    err = pthread_attr_setstacksize(&threadAttr, stackSize);
    if (err) throw std::runtime_error(to_string(errno) + ": can't set stack size");

    InvocationArgs args = {
            .invoker = invoker,
            .app_id = app_id,
            .request = request,
            .execTime = execTime,
            .session = Executor::getSession()
    };

    err = pthread_create(&threadID, &threadAttr, threadStart, &args);
    if (err) throw std::runtime_error(to_string(errno) + ": thread creation failed");

    pthread_attr_destroy(&threadAttr);
    // We ignore errors here

    void* retPtr;
    err = pthread_join(threadID, &retPtr);
    int ret = err ? int(StatusCode::internal_error) : *(int*) retPtr;

    free(retPtr);
    return ret;
}

/*
void Executor::executeAll(int workersCount = -1) {
    using namespace std;
    workersCount = workersCount == -1 ? (int) thread::hardware_concurrency() : workersCount;

    thread pool[workersCount];
    for (auto& worker: pool) {
        worker = thread(&Executor::worker, this);
    }

    for (auto& worker: pool) {
        worker.join();
    }
}*/

static inline
int64_t calculateExternalGas(int64_t currentGas) {
    // the geometric series approaches 1 / (1 - q) so the total amount of externalGas would be 2 * currentGas
    return 2 * currentGas / 3;
}

#define MIN_GAS 1

Executor::CallResourceContext::CallResourceContext(byte forwardedGas) {
    prevResources = session->currentResources;
    caller = session->currentCall->appID;

    gas = (prevResources->remainingExternalGas * forwardedGas) >> 8;
    if (gas <= MIN_GAS) throw ApplicationError("forwarded gas is too low", StatusCode::invalid_operation);

    id = session->failureManager.nextInvocation();
    prevResources->remainingExternalGas -= gas;
    remainingExternalGas = calculateExternalGas(gas);

    session->currentResources = this;
    heapVersion = session->heapModifier.saveVersion();
}


Executor::CallResourceContext::CallResourceContext(int_fast32_t initialGas) {
    id = session->failureManager.nextInvocation();
    caller = 0;
    gas = 0;
    remainingExternalGas = initialGas;

    session->currentResources = this;
    heapVersion = 0;
}

Executor::CallResourceContext::~CallResourceContext() noexcept {
    blockSignals();
    session->currentResources = prevResources;
    session->failureManager.completeInvocation();
    if (!completed) {
        session->heapModifier.restoreVersion(heapVersion);
    }
    // CallInfoContext's destructor restores the caller's context, but we also do this here to make sure.
    session->heapModifier.loadContext(caller);
    unBlockSignals();
}

Executor::CallInfoContext::CallInfoContext(long_id app) : appID(app) {
    prevCallInfo = session->currentCall;
    if (prevCallInfo->appID == app) throw GenericError("calling self", StatusCode::invalid_operation, app);
    session->heapModifier.loadContext(app);
    session->currentCall = this;
}

Executor::CallInfoContext::CallInfoContext() : appID(0) {
    prevCallInfo = nullptr;
    session->currentCall = this;
}

Executor::CallInfoContext::~CallInfoContext() noexcept {
    blockSignals();
    argc::exit_area();

    // restore context
    if (prevCallInfo != nullptr) session->heapModifier.loadContext(prevCallInfo->appID);

    // restore previous call info
    session->currentCall = prevCallInfo;
    unBlockSignals();
}

