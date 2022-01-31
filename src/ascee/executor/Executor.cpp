// Copyright (c) 2021-2022 aybehrouz <behrouz_ayati@yahoo.com>. All rights
// reserved. This file is part of the C++ implementation of the Argennon smart
// contract Execution Environment (AscEE).
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

#include <pthread.h>
#include <csignal>
#include <argc/functions.h>
#include <argc/types.h>
#include <thread>
#include "Executor.h"

using namespace argennon;
using namespace ascee::runtime;
using std::unique_ptr, std::string, std::to_string, std::function;

thread_local Executor::SessionInfo* Executor::session = nullptr;
bool Executor::initialized = false;

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

static
void* registerRecoveryStack() {
    stack_t sig_stack;

    sig_stack.ss_sp = malloc(SIGSTKSZ);
    if (sig_stack.ss_sp == nullptr) throw std::runtime_error("could not allocate memory for the recovery stack");

    sig_stack.ss_size = SIGSTKSZ;
    sig_stack.ss_flags = 0;

    int err = sigaltstack(&sig_stack, nullptr);
    if (err) throw std::runtime_error("error in registering the recovery stack");

    return sig_stack.ss_sp;
}

void Executor::sig_handler(int sig, siginfo_t* info, void*) {
    printf("Caught signal %d\n", sig);
    // important: session is not valid when sig == SIGALRM
    if (sig != SIGALRM) {
        if (session->guardedArea) {
            std::cerr << "A signal was raised from guarded area." << std::endl;
            std::terminate();
        }
        int ret = static_cast<int>(StatusCode::internal_error);
        if (sig == SIGSEGV) ret = static_cast<int>(StatusCode::memory_fault);
        if (sig == SIGUSR1) ret = static_cast<int>(StatusCode::execution_timeout);
        if (sig == SIGFPE) ret = static_cast<int>(StatusCode::arithmetic_error);
        siglongjmp(session->currentCall->env, ret);
    } else {
        pthread_t thread_id = *static_cast<pthread_t*>(info->si_value.sival_ptr);
        printf("Caught signal %d for app %ld\n", sig, thread_id);
        pthread_kill(thread_id, SIGUSR1);
    }
}

void Executor::initHandlers() {
    struct sigaction action{};
    action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    action.sa_sigaction = sig_handler;
    sigfillset(&action.sa_mask);

    int err = 0;
    err += sigaction(SIGALRM, &action, nullptr);
    err += sigaction(SIGFPE, &action, nullptr);
    err += sigaction(SIGSEGV, &action, nullptr);
    err += sigaction(SIGBUS, &action, nullptr);
    err += sigaction(SIGUSR1, &action, nullptr);
    if (err) throw std::runtime_error("error in creating handlers");
}

/**
 * The correct usage of this function is to use it at the start of the critical area and only call unGuard() when the
 * code is completed normally. For example:
 * @code
 * int f() {
 *     guardArea();
 *
 *     // no unGuard should be called here.
 *     throw std::exception();
 *
 *     unGuard();
 *     return 0;
 * }
 */
void Executor::guardArea() {
    session->guardedArea = true;
    maskSignals(SIG_BLOCK);
}

void Executor::unGuard() {
    maskSignals(SIG_UNBLOCK);
    session->guardedArea = false;
}

// must be thread-safe
AppResponse Executor::executeOne(AppRequest* req) {
    // Do not call initialize() here. It can break thread-safety
    if (!initialized) throw std::runtime_error("Executor not initialized");
    response_buffer_c response;
    int statusCode;
    session = nullptr;
    try {
        SessionInfo threadSession{
                .request = req,
        };
        session = &threadSession;

        CallResourceHandler rootResourceCtx(req->gas);
        CallContext rootInfoCtx;
        statusCode = argc::invoke_dispatcher(255, req->calledAppID, response, string_view_c(req->httpRequest));
        rootResourceCtx.complete();
        session->heapModifier.writeToHeap();
    } catch (const std::out_of_range& err) {
        //todo: fix this
        throw std::runtime_error("why???");
    }
    session = nullptr;

    return {statusCode, string(response)};
}

struct InvocationArgs {
    const function<int(long_id, ascee::response_buffer_c&, ascee::string_view_c)>& invoker;

    long_id app_id;
    ascee::string_view_c request;
    ascee::response_buffer_c& response;
    int_fast64_t execTime;
    Executor::SessionInfo* session;
};

void* Executor::threadStart(void* voidArgs) {
    auto* args = (InvocationArgs*) voidArgs;

    void* recoveryStack = registerRecoveryStack();
    session = args->session;

    int* ret = (int*) malloc(sizeof(int));
    if (ret == nullptr) throw std::runtime_error("memory allocation error");

    ThreadCpuTimer cpuTimer;
    cpuTimer.setAlarm(args->execTime);

    *ret = args->invoker(args->app_id, args->response, args->request);

    free(recoveryStack);
    return ret;
}

int Executor::controlledExec(const function<int(long_id, response_buffer_c&, string_view_c)>& invoker,
                             long_id app_id,
                             response_buffer_c& response, string_view_c request,
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
            .response = response,
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

void Executor::initialize() {
    if (!initialized) {
        initHandlers();
        initialized = true;
    }
}

static inline
int64_t calculateExternalGas(int64_t currentGas) {
    // the geometric series approaches 1 / (1 - q) so the total amount of externalGas would be 2 * currentGas
    return 2 * currentGas / 3;
}

#define MIN_GAS 1

Executor::CallResourceHandler::CallResourceHandler(byte forwardedGas) {
    prevResources = session->currentResources;
    caller = session->currentCall->appID;

    gas = (prevResources->remainingExternalGas * forwardedGas) >> 8;
    if (gas <= MIN_GAS) throw AsceeError("forwarded gas is too low", StatusCode::invalid_operation);

    id = session->failureManager.nextInvocation();
    prevResources->remainingExternalGas -= gas;
    remainingExternalGas = calculateExternalGas(gas);

    session->currentResources = this;
    heapVersion = session->heapModifier.saveVersion();
}

Executor::CallResourceHandler::CallResourceHandler(int_fast32_t initialGas) {
    id = session->failureManager.nextInvocation();
    caller = 0;
    gas = 0;
    remainingExternalGas = initialGas;

    session->currentResources = this;
    heapVersion = 0;
}

Executor::CallResourceHandler::~CallResourceHandler() noexcept {
    guardArea();
    session->currentResources = prevResources;
    session->failureManager.completeInvocation();
    if (!completed) {
        session->heapModifier.restoreVersion(heapVersion);
    }
    // CallInfoContext's destructor restores the caller's context, but we also do this here to make sure.
    session->heapModifier.loadContext(caller);
    unGuard();
}

Executor::CallContext::CallContext(long_id app) : appID(app) {
    prevCallInfo = session->currentCall;
    if (prevCallInfo->appID == app) throw Error("calling self", StatusCode::invalid_operation, app);
    session->heapModifier.loadContext(app);
    session->currentCall = this;
}

Executor::CallContext::CallContext() : appID(0) {
    prevCallInfo = nullptr;
    session->currentCall = this;
}

Executor::CallContext::~CallContext() noexcept {
    guardArea();
    argc::exit_area();

    // restore context
    if (prevCallInfo != nullptr) session->heapModifier.loadContext(prevCallInfo->appID);

    // restore previous call info
    session->currentCall = prevCallInfo;
    unGuard();
}

