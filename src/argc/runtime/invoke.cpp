
#include <csignal>
#include <cstdlib>

#include <memory>
#include <unordered_map>
#include <vector>
#include <csetjmp>

#include <Executor.h>
#include <argc/functions.h>
#include "heap/HeapModifier.h"
#include "loader/AppLoader.h"


using std::unique_ptr, std::vector, std::unordered_map;
using namespace ascee;

static
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

static
void blockSignals() {
    maskSignals(SIG_BLOCK);
    Executor::getSession()->criticalArea = true;
}

static
void unBlockSignals() {
    maskSignals(SIG_UNBLOCK);
    Executor::getSession()->criticalArea = false;
}

/// This function must not return zero, and instead of zero it should return a small positive value.
static
int64_t calculateMaxExecTime(int64_t max_cost) {
    int64_t ret = max_cost * 1000000;
    return (ret == 0) ? 1000 : ret;
}

// todo: small function should be defined as inline
extern "C"
string_buffer* argcrt::getResponse() {
    return &Executor::getSession()->response;
}

extern "C"
void argcrt::enter_area() {
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
    Executor::getSession()->currentCall->deferredCalls.push_back(
            std::make_unique<DeferredArgs>(DeferredArgs{
                    .appID = app_id,
                    .forwardedGas = forwarded_gas,
                    .request = std::string_view(request.content, request.length),
            }));
}

extern "C"
int argcrt::invoke_dispatcher(byte forwarded_gas, std_id_t app_id, string_t request) {
    auto dispatcher = AppLoader::getDispatcher(app_id);
    if (dispatcher == nullptr || Executor::getSession()->currentCall->appID == app_id) {
        return NOT_FOUND;
    }
    blockSignals();
    int maxCurrentGas = (Executor::getSession()->currentCall->remainingExternalGas * forwarded_gas) >> 8;

    Executor::getSession()->currentCall->remainingExternalGas -= maxCurrentGas;
    auto oldCallInfo = Executor::getSession()->currentCall;
    SessionInfo::CallContext newCall = {
            .appID = app_id,
            .remainingExternalGas = maxCurrentGas / 2,
    };
    Executor::getSession()->currentCall = &newCall;
    Executor::getSession()->response.end = 0;
    Executor::getSession()->heapModifier->openContext(app_id);

    int64_t remainingExecTime = Executor::getSession()->cpuTimer.setAlarm(calculateMaxExecTime(maxCurrentGas));

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
    exit_area();
    Executor::getSession()->recentEnvPointer = oldEnv;
    Executor::getSession()->cpuTimer.setAlarm(remainingExecTime);

    if (ret < BAD_REQUEST) {
        for (const auto& dCall: Executor::getSession()->currentCall->deferredCalls) {
            int temp = invoke_dispatcher(
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

    if (ret >= BAD_REQUEST) {
        Executor::getSession()->heapModifier->closeContextAbruptly(app_id, oldCallInfo->appID);
    } else {
        Executor::getSession()->heapModifier->closeContextNormally(app_id, oldCallInfo->appID);
    }

    // restore previous call info
    Executor::getSession()->currentCall = oldCallInfo;
    unBlockSignals();
    return ret;
}
