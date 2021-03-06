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
using namespace ascee;
using namespace runtime;
using std::unique_ptr, std::string, std::string_view, std::to_string, std::function;

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

void Executor::sig_handler(int sig, siginfo_t* info, void*) {
    printf("Caught signal %d\n", sig);
    // important: session is not valid when sig == SIGALRM
    if (sig != SIGALRM) {
        if (session->guardedArea) {
            printf("A signal was raised from guarded area.\n");
            std::terminate();
        }
        int ret = static_cast<int>(StatusCode::internal_error);
        if (sig == SIGSEGV) ret = static_cast<int>(StatusCode::memory_fault);
        else if (sig == SIGUSR1) ret = static_cast<int>(StatusCode::execution_timeout);
        else if (sig == SIGFPE || sig == SIGILL) ret = static_cast<int>(StatusCode::arithmetic_error);
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
    err += sigaction(SIGILL, &action, nullptr);
    err += sigaction(SIGSEGV, &action, nullptr);
    err += sigaction(SIGBUS, &action, nullptr);
    err += sigaction(SIGUSR1, &action, nullptr);
    if (err) throw std::runtime_error("error in creating handlers");
}

void Executor::ControlledCaller::guardArea_() {
    session->guardedArea = true;
    maskSignals(SIG_BLOCK);
}

void Executor::ControlledCaller::unGuard_() {
    maskSignals(SIG_UNBLOCK);
    session->guardedArea = false;
}

// must be thread-safe
AppResponse Executor::executeOne(AppRequest* req) {
    response_buffer_c response;
    int statusCode;
    session = nullptr;
    try {
        SessionInfo threadSession{
                .request = req,
                //.cryptoSigner = cryptoSigner,
        };
        session = &threadSession;
        if (req->useControlledExecution) {
            session->callManager = std::make_unique<ControlledCaller>();
            ControlledCaller::CallResourceHandler rootResourceCtx(req->maxClocks);
            CallContext rootInfoCtx;
            statusCode = callApp(255, req->calledAppID, response, string_view_c(req->httpRequest));
            rootResourceCtx.complete();
        } else {
            session->callManager = std::make_unique<OptimisticCaller>();
            ThreadCpuTimer cpuTimer;
            cpuTimer.setAlarm(default_exec_time_nsec);
            CallContext rootInfoCtx;
            statusCode = callApp(255, req->calledAppID, response, string_view_c(req->httpRequest));
        }
        session->heapModifier.writeToHeap();
    } catch (const std::out_of_range& err) {
        //todo: fix this
        throw std::runtime_error("why???");
    }
    session = nullptr;

    return {statusCode, string((StringView) response)};
}

int Executor::callApp(byte forwarded_gas, long_id app_id, response_buffer_c& response, string_view_c request) {
    return session->callManager->executeApp(forwarded_gas, app_id, response, request);
}

Executor::Executor() {
    if (!initialized.exchange(true)) initHandlers();
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


struct InvocationArgs {
    long_id app_id;
    ascee::string_view_c request;
    ascee::response_buffer_c& response;
    int_fast64_t execTime;
    Executor::SessionInfo* session;
};

static
int invoke_noexcept(long_id app_id, response_buffer_c& response, string_view_c request) {
    int ret;
    try {
        ret = argc::dependant_call(app_id, response, request);
    } catch (const Executor::Error& ee) {
        ret = ee.errorCode();
        ee.toHttpResponse(response.clear());
    }
    return ret;
}

void* Executor::ControlledCaller::threadStart(void* voidArgs) {
    auto* args = (InvocationArgs*) voidArgs;

    void* recoveryStack = registerRecoveryStack();
    session = args->session;

    int* ret = (int*) malloc(sizeof(int));
    if (ret == nullptr) throw std::runtime_error("memory allocation error");

    ThreadCpuTimer cpuTimer;
    cpuTimer.setAlarm(args->execTime);

    *ret = invoke_noexcept(args->app_id, args->response, args->request);

    free(recoveryStack);
    return ret;
}

int Executor::ControlledCaller::executeApp(byte forwarded_gas, long_id app_id,
                                           response_buffer_c& response, string_view_c request) {
    guardArea_();
    int ret;
    try {
        CallResourceHandler resourceContext(forwarded_gas);
        pthread_t threadID;
        pthread_attr_t threadAttr;

        int err = pthread_attr_init(&threadAttr);
        if (err) throw std::runtime_error(to_string(errno) + ": failed to init thread attributes");

        err = pthread_attr_setstacksize(&threadAttr, resourceContext.getStackSize());
        if (err) throw std::runtime_error(to_string(errno) + ": can't set stack size");

        InvocationArgs args = {
                .app_id = app_id,
                .request = request,
                .response = response,
                .execTime = resourceContext.getExecTime(),
                .session = Executor::getSession()
        };

        err = pthread_create(&threadID, &threadAttr, threadStart, &args);
        if (err) throw std::runtime_error(to_string(errno) + ": thread creation failed");

        pthread_attr_destroy(&threadAttr);
        // We ignore errors here

        void* retPtr;
        err = pthread_join(threadID, &retPtr);
        ret = err ? int(StatusCode::internal_error) : *(int*) retPtr;

        free(retPtr);
        if (ret < 400) resourceContext.complete();
    } catch (const AsceeError& ae) {
        ret = ae.errorCode();
        Executor::Error(ae).toHttpResponse(response.clear());
    }
    // unBlockSignals() should be called here, in case resourceContext's constructor throws an exception.
    unGuard_();
    return ret;
}

static inline
int64_t calculateExternalGas(int64_t currentGas) {
    // the geometric series approaches 1 / (1 - q) so the total amount of externalGas would be 2 * currentGas
    return 2 * currentGas / 3;
}


Executor::ControlledCaller::CallResourceHandler::CallResourceHandler(byte forwardedGas) {
    prevResources = session->currentResources;
    caller = session->currentCall->appID;

    gas = (prevResources->remainingExternalGas * forwardedGas) >> 8;
    if (gas <= min_clocks) throw AsceeError("forwarded gas is too low", StatusCode::invalid_operation);

    id = session->failureManager.nextInvocation();
    prevResources->remainingExternalGas -= gas;
    remainingExternalGas = calculateExternalGas(gas);

    session->currentResources = this;
    heapVersion = session->heapModifier.saveVersion();
}

Executor::ControlledCaller::CallResourceHandler::CallResourceHandler(int_fast32_t initialGas) {
    id = session->failureManager.nextInvocation();
    caller = 0;
    gas = 0;
    remainingExternalGas = initialGas;
    session->currentResources = this;
    heapVersion = 0;
}

Executor::ControlledCaller::CallResourceHandler::~CallResourceHandler() noexcept {
    Executor::guardArea();
    session->currentResources = prevResources;
    session->failureManager.completeInvocation();
    if (!completed) {
        session->heapModifier.restoreVersion(heapVersion);
    }
    // CallInfoContext's destructor restores the caller's context, but we also do this here to make sure.
    session->heapModifier.loadContext(caller);
    Executor::unGuard();
}

Executor::OptimisticCaller::CallResourceHandler::CallResourceHandler() {
    caller = session->currentCall->appID;
    heapVersion = session->heapModifier.saveVersion();
}

Executor::OptimisticCaller::CallResourceHandler::~CallResourceHandler() noexcept {
    Executor::guardArea();
    if (!completed) {
        session->heapModifier.restoreVersion(heapVersion);
    }
    // CallInfoContext's destructor restores the caller's context, but we also do this here to make sure.
    session->heapModifier.loadContext(caller);
    Executor::unGuard();
}

int Executor::OptimisticCaller::executeApp(byte forwarded_gas, long_id app_id,
                                           response_buffer_c& response, string_view_c request) {
    guardArea_();
    try {
        CallResourceHandler resourceContext;
        auto ret = argc::dependant_call(app_id, response, request);
        if (ret < 400) resourceContext.complete();
        unGuard_();
        return ret;
    } catch (const AsceeError& ae) {
        throw BlockError("an optimistic call failed");
    }
}


