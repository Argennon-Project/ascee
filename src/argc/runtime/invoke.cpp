
#include <csignal>

#include <memory>
#include <unordered_map>
#include <vector>
#include <csetjmp>

#include <Executor.h>
#include <argc/functions.h>
#include "heap/HeapModifier.h"

#define MIN_EXEC_TIME_NSEC 10000

using std::unique_ptr, std::vector, std::unordered_map;
using namespace ascee;
using namespace argc;

static inline
int64_t calculateExternalGas(int64_t currentGas) {
    // the geometric series approaches 1 / (1 - q) so the total amount of externalGas would be 2 * currentGas
    return 2 * currentGas / 3;
}

static
int64_t calculateMaxExecTime(int64_t max_cost) {
    return max_cost * 1000000;
}

static inline
void addDefaultResponse(int statusCode) {
    char response[200];
    int n = sprintf(response, "HTTP/1.1 %d %s", statusCode, "OK");
    Executor::getSession()->response.end = 0;
    argc::append_str(&Executor::getSession()->response, String{response, n + 1});
}

static inline
void maskSignals(int how) {
    sigset_t set;

// Block timer's signal
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGABRT);
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
string_buffer* argc::response_buffer() {
    return &Executor::getSession()->response;
}

extern "C"
void argc::enter_area() {
    if (Executor::getSession()->currentCall->hasLock) return;
    blockSignals();
    if (Executor::getSession()->isLocked[Executor::getSession()->currentCall->appID]) {
        raise(SIGUSR2);
    } else {
        Executor::getSession()->isLocked[Executor::getSession()->currentCall->appID] = true;
        Executor::getSession()->currentCall->hasLock = true;
    }
    unBlockSignals();
}

extern "C"
void argc::exit_area() {
    blockSignals();
    if (Executor::getSession()->currentCall->hasLock) {
        Executor::getSession()->isLocked[Executor::getSession()->currentCall->appID] = false;
        Executor::getSession()->currentCall->hasLock = false;
    }
    unBlockSignals();
}

extern "C"
void argc::invoke_deferred(byte forwarded_gas, std_id_t app_id, string_t request) {
    Executor::getSession()->currentCall->deferredCalls.push_back(
            std::make_unique<DeferredArgs>(DeferredArgs{
                    .appID = app_id,
                    .forwardedGas = forwarded_gas,
                    .request = std::string_view(request.content, request.length),
            }));
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
    int64_t execTime = calculateMaxExecTime(maxCurrentGas);
    if (execTime <= MIN_EXEC_TIME_NSEC) return REQUEST_TIMEOUT;

    Executor::getSession()->currentCall->remainingExternalGas -= maxCurrentGas;
    auto oldCallInfo = Executor::getSession()->currentCall;
    SessionInfo::CallContext newCall = {
            .appID = app_id,
            .remainingExternalGas = calculateExternalGas(maxCurrentGas),
    };
    Executor::getSession()->currentCall = &newCall;
    Executor::getSession()->response.end = 0;
    Executor::getSession()->heapModifier->openContext(app_id);

    int64_t remainingExecTime = Executor::getSession()->cpuTimer.setAlarm(execTime);

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

    // as soon as possible we should release the entrance lock of the app (if any) and restore the old env value
    argc::exit_area();
    Executor::getSession()->recentEnvPointer = oldEnv;
    Executor::getSession()->cpuTimer.setAlarm(remainingExecTime);

    if (ret < BAD_REQUEST) {
        for (const auto& dCall: Executor::getSession()->currentCall->deferredCalls) {
            int temp = argc::invoke_dispatcher(
                    dCall->forwardedGas,
                    dCall->appID,
                    // we should NOT use length + 1 here.
                    string_t{dCall->request.data(), static_cast<int>(dCall->request.size())}
            );
            if (temp >= BAD_REQUEST) {
                ret = FAILED_DEPENDENCY;
                break;
            }
        }
    }

    if (ret >= 400) {
        Executor::getSession()->heapModifier->closeContextAbruptly(app_id, oldCallInfo->appID);
    } else {
        Executor::getSession()->heapModifier->closeContextNormally(app_id, oldCallInfo->appID);
    }

    // restore previous call info
    Executor::getSession()->currentCall = oldCallInfo;
    return ret;
}

extern "C"
int argc::invoke_dispatcher(byte forwarded_gas, std_id_t app_id, string_t request) {
    blockSignals();
    int ret = invoke_dispatcher_impl(forwarded_gas, app_id, request);
    if (ret >= 400) addDefaultResponse(ret);
    unBlockSignals();
    return ret;
}

