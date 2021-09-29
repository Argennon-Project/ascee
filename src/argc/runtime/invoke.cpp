
#include <pthread.h>
#include <ctime>
#include <csignal>
#include <cstdlib>

#include <memory>
#include <unordered_map>
#include <vector>
#include <csetjmp>

#include "session.h"
#include "../../heap/HeapModifier.h"
#include "../../heap/Heap.h"
#include "../../loader/AppLoader.h"


using std::string, std::unique_ptr, std::vector, std::unordered_map;
using namespace ascee;

struct DispatcherArgs {
    SessionInfo* session;
    int maxGas;
    string_t& request;
};

static
void maskSignals(int how) {
    sigset_t set;

// Block timer's signal
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGABRT);
    int s = pthread_sigmask(how, &set, nullptr);
    if (s != 0) throw std::runtime_error("error in masking signals");
}

static
void blockSignals() {
    maskSignals(SIG_BLOCK);
}

static
void unBlockSignals() {
    maskSignals(SIG_UNBLOCK);
}

// todo: small function should be defined as inline
extern "C"
string_buffer* getResponse() {
    return &session->responseBuffer;
}

extern "C"
void enter_area() {
    if (session->currentCall->hasLock) return;
    blockSignals();
    if (session->isLocked[session->currentCall->appID]) {
        raise(SIGUSR2);
    } else {
        session->isLocked[session->currentCall->appID] = true;
        session->currentCall->hasLock = true;
    }
    unBlockSignals();
}

extern "C"
void exit_area() {
    blockSignals();
    if (session->currentCall->hasLock) {
        session->isLocked[session->currentCall->appID] = false;
        session->currentCall->hasLock = false;
    }
    unBlockSignals();
}

static
int64_t calculate_max_time(int64_t max_cost) {
    return max_cost * 1000000;
}

extern "C"
void invoke_deferred(byte forwarded_gas, std_id_t app_id, string_t request) {
    session->currentCall->deferredCalls.push_back(
            std::make_unique<DeferredArgs>(DeferredArgs{
                    .appID = app_id,
                    .forwardedGas = forwarded_gas,
                    .request = string(request.content, request.length),
            }));
}

extern "C"
int invoke_dispatcher(byte forwarded_gas, std_id_t app_id, string_t request) {
    auto dispatcher = AppLoader::getDispatcher(app_id);
    if (dispatcher == nullptr || session->currentCall->appID == app_id) {
        return NOT_FOUND;
    }
    blockSignals();
    int maxCurrentGas = (session->currentCall->remainingExternalGas * forwarded_gas) >> 8;

    session->currentCall->remainingExternalGas -= maxCurrentGas;
    auto oldCallInfo = session->currentCall;
    SessionInfo::CallContext newCall = {
            .appID = app_id,
            .remainingExternalGas = maxCurrentGas / 2,
    };
    session->currentCall = &newCall;
    session->responseBuffer.end = 0;
    session->heapModifier->openContext(app_id);

    int64_t remainingExecTime = session->cpuTimer.setAlarm(calculate_max_time(maxCurrentGas));

    jmp_buf* oldEnv = session->envPointer;
    jmp_buf env;
    session->envPointer = &env;

    int ret, jmpRet = sigsetjmp(env, 1);
    if (jmpRet == 0) {
        unBlockSignals();
        ret = dispatcher(request);
        blockSignals();
    } else {
        // because we have used sigsetjmp and saved the signal mask, we don't need to call blockSignals() here.
        ret = jmpRet;
    }

    // as soon as possible we should release the entrance lock of the app (if any) and restore the old env value
    exit_area();
    session->envPointer = oldEnv;
    session->cpuTimer.setAlarm(remainingExecTime);

    if (ret < BAD_REQUEST) {
        for (const auto& dCall: session->currentCall->deferredCalls) {
            int temp = invoke_dispatcher(
                    dCall->forwardedGas,
                    dCall->appID,
                    // we should NOT use length + 1 here.
                    string_t{dCall->request.data(), static_cast<int>(dCall->request.length())}
            );
            if (temp >= BAD_REQUEST) {
                ret = FAILED_DEPENDENCY;
                break;
            }
        }
    }

    if (ret >= BAD_REQUEST) {
        session->heapModifier->closeContextAbruptly(app_id, oldCallInfo->appID);
    } else {
        session->heapModifier->closeContextNormally(app_id, oldCallInfo->appID);
    }

    // restore previous call info
    session->currentCall = oldCallInfo;
    unBlockSignals();
    return ret;
}

void* registerRecoveryStack() {
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

void executeSession(int transactionInfo) {
    void* p = registerRecoveryStack();
    SessionInfo::CallContext newCall = {
            .appID = 0,
            .remainingExternalGas = 2000000,
    };
    SessionInfo threadSession{
            .heapModifier = unique_ptr<HeapModifier>(Heap::setupSession(transactionInfo)),
            .currentCall = &newCall,
    };
    session = &threadSession;
    int ret = invoke_dispatcher(50, 1, String{"Hey!", 5});
    std::cout << "returned: " << ret << std::endl;
    free(p);
}

