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
#include <csetjmp>
#include <memory>
#include <unordered_map>
#include <vector>

#include <Executor.h>
#include <argc/functions.h>

#define MIN_GAS 1

using std::unique_ptr, std::vector, std::unordered_map, std::string;
using namespace ascee;

static inline
int64_t calculateExternalGas(int64_t currentGas) {
    // the geometric series approaches 1 / (1 - q) so the total amount of externalGas would be 2 * currentGas
    return 2 * currentGas / 3;
}

static inline
void addDefaultResponse(int statusCode) {
    char response[256];
    int n = sprintf(response, "HTTP/1.1 %d %s", statusCode, "OK");
    Executor::getSession()->response.end = 0;
    argcrt::append_str(&Executor::getSession()->response, string_t{response, n});
}

static inline
void maskSignals(int how) {
    sigset_t set;

// Block timer's signal
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGKILL);
    sigaddset(&set, SIGBUS);
    int s = pthread_sigmask(how, &set, nullptr);
    if (s != 0) throw std::runtime_error("error in masking signals");

}

static inline
void blockSignals() {
    maskSignals(SIG_BLOCK);
    Executor::getSession()->criticalArea = true;
}

static inline
void unBlockSignals() {
    maskSignals(SIG_UNBLOCK);
    Executor::getSession()->criticalArea = false;
}


extern "C"
string_buffer* argcrt::response_buffer() {
    return &Executor::getSession()->response;
}

extern "C"
void argcrt::enter_area() {
    if (Executor::getSession()->currentCall->hasLock) return;

    if (Executor::getSession()->isLocked[Executor::getSession()->currentCall->appID]) {
        raise(SIGUSR2);
    } else {
        blockSignals();
        Executor::getSession()->isLocked[Executor::getSession()->currentCall->appID] = true;
        Executor::getSession()->currentCall->hasLock = true;
        unBlockSignals();
    }
}

extern "C"
void argcrt::exit_area() {
    blockSignals();
    if (Executor::getSession()->currentCall->hasLock) {
        Executor::getSession()->isLocked[Executor::getSession()->currentCall->appID] = false;
        Executor::getSession()->currentCall->hasLock = false;
    }
    unBlockSignals();
}

extern "C"
void argcrt::invoke_deferred(byte forwarded_gas, std_id_t app_id, string_t request) {
    Executor::getSession()->currentCall->deferredCalls.emplace_back(DeferredArgs{
            .appID = app_id,
            .forwardedGas = forwarded_gas,
            // string constructor makes a copy of its input, so we should be safe here.
            .request = std::string(request.content, request.length),
    });
}

static inline
int invoke_dispatcher_impl(byte forwarded_gas, std_id_t app_id, string_t request) {
    dispatcher_ptr_t dispatcher;
    try {
        dispatcher = Executor::getSession()->appTable.at(app_id);
    } catch (const std::out_of_range&) {
        return PRECONDITION_FAILED;
    }
    if (dispatcher == nullptr || Executor::getSession()->currentCall->appID == app_id) return NOT_FOUND;

    int64_t maxCurrentGas = (Executor::getSession()->currentCall->remainingExternalGas * forwarded_gas) >> 8;
    if (maxCurrentGas <= MIN_GAS) return REQUEST_TIMEOUT;

    int64_t execTime = Executor::getSession()->failureManager.getExecTime(maxCurrentGas);

    int16_t savedVersion;
    try {
        savedVersion = Executor::getSession()->heapModifier->saveVersion();
    } catch (const std::out_of_range&) {
        return INSUFFICIENT_STORAGE;
    }

    Executor::getSession()->currentCall->remainingExternalGas -= maxCurrentGas;
    auto oldCallInfo = Executor::getSession()->currentCall;
    SessionInfo::CallContext newCall = {
            .appID = app_id,
            .remainingExternalGas = calculateExternalGas(maxCurrentGas),
    };
    Executor::getSession()->currentCall = &newCall;
    Executor::getSession()->response.end = 0;
    Executor::getSession()->heapModifier->loadContext(app_id);

    ascee::ThreadCpuTimer cpuTimer;
    cpuTimer.setAlarm(execTime);

    jmp_buf* oldEnv = Executor::getSession()->recentEnvPointer;
    jmp_buf env;
    Executor::getSession()->recentEnvPointer = &env;

    int ret, jmpRet = sigsetjmp(env, 1);
    if (jmpRet == 0) {
        unBlockSignals();
        ret = dispatcher(request);
    } else {
        // because we have used sigsetjmp and saved the signal mask, we don't need to call blockSignals() here.
        ret = jmpRet;
    }
    // blockSignals() activates the criticalArea flag. That way if we raise another signal here, we won't get stuck in
    // an infinite loop.
    blockSignals();

    // as soon as possible we should restore the old env value and release the entrance lock (if any)
    Executor::getSession()->recentEnvPointer = oldEnv;
    argcrt::exit_area();

    if (ret < 400) {
        string mainResponse = string(argcrt::response_buffer()->buffer, argcrt::response_buffer()->end);

        for (const auto& dCall: Executor::getSession()->currentCall->deferredCalls) {
            int temp = argcrt::invoke_dispatcher(
                    dCall.forwardedGas,
                    dCall.appID,
                    // we should NOT use length + 1 here.
                    string_t{dCall.request.c_str(), static_cast<int>(dCall.request.length())}
            );
            printf("** deferred call return: %d\n", temp);
            if (temp >= BAD_REQUEST) {
                ret = FAILED_DEPENDENCY;
                break;
            }
        }

        argcrt::response_buffer()->end = 0;
        argcrt::append_str(argcrt::response_buffer(),
                           string_t{mainResponse.c_str(), static_cast<int32>(mainResponse.length())});
    }

    // restore context
    Executor::getSession()->heapModifier->loadContext(oldCallInfo->appID);
    if (ret >= 400) {
        Executor::getSession()->heapModifier->restoreVersion(savedVersion);
    }

    // restore previous call info
    Executor::getSession()->currentCall = oldCallInfo;
    return ret;
}


extern "C"
int argcrt::invoke_dispatcher(byte forwarded_gas, std_id_t app_id, string_t request) {
    blockSignals();

    Executor::getSession()->failureManager.nextInvocation();
    int ret;
    try {
        auto stackSize = Executor::getSession()->failureManager.getStackSize();
        ret = Executor::controlledExec(invoke_dispatcher_impl, forwarded_gas, app_id, request, stackSize);
    } catch (const std::overflow_error&) {
        ret = MAX_CALL_DEPTH_REACHED;
    }

    if (ret >= 400) addDefaultResponse(ret);

    Executor::getSession()->failureManager.completeInvocation();

    unBlockSignals();
    return ret;
}

