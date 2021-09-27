
#include <pthread.h>
#include <ctime>
#include <csignal>
#include <cstdlib>

#include <memory>
#include <unordered_map>
#include <vector>

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

// Block SIGALRM;
    sigemptyset(&set);
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
string_buffer* getResponse(SessionInfo* session) {
    return &session->responseBuffer;
}

extern "C"
void enter_area(SessionInfo* session) {
    if (session->currentCall->hasLock) return;

    if (session->isLocked[session->currentCall->appID]) {
        raise(SIGUSR1);
    } else {
        blockSignals();
        session->isLocked[session->currentCall->appID] = true;
        session->currentCall->hasLock = true;
        unBlockSignals();
    }
}

extern "C"
void exit_area(SessionInfo* session) {
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

static
void* callerThread(void* arg) {
    auto* arguments = static_cast<DispatcherArgs*>(arg);

    // Creating the execution timer
    timer_t exec_timer;
    struct sigevent sev{};
    struct itimerspec its{};
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGALRM;
    pthread_t thread_id = pthread_self();
    sev.sigev_value.sival_ptr = &thread_id;
    if (timer_create(CLOCK_THREAD_CPUTIME_ID, &sev, &exec_timer) != 0) {
        throw std::runtime_error("error in creating the cpu timer");
    }

    // Start the timer
    auto time_nsec = calculate_max_time(arguments->maxGas);
    its.it_value.tv_sec = time_nsec / 1000000000;
    its.it_value.tv_nsec = time_nsec % 1000000000;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    if (timer_settime(exec_timer, 0, &its, nullptr) != 0) {
        throw std::runtime_error("error in starting the timer");
    }

    auto dispatcher = AppLoader::getDispatcher(arguments->session->currentCall->appID);
    unBlockSignals();
    int ret = (dispatcher == nullptr) ? NOT_FOUND : dispatcher(arguments->session, arguments->request);
    blockSignals();

    // we don't let an app manually return REQUEST_TIMEOUT
    if (ret == REQUEST_TIMEOUT) ret = INTERNAL_ERROR;

    int* retMem = (int*) malloc(sizeof(int));
    *retMem = ret;
    return retMem;
}

extern "C"
void invoke_deferred(SessionInfo* session, byte forwarded_gas, std_id_t app_id, string_t request) {
    session->currentCall->deferredCalls.push_back(
            std::make_unique<DeferredArgs>(DeferredArgs{
                    .appID = app_id,
                    .forwardedGas = forwarded_gas,
                    .request = string(request.content, request.length),
            }));
}

extern "C"
int invoke_dispatcher(SessionInfo* session, byte forwarded_gas, std_id_t app_id, string_t request) {
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

    DispatcherArgs args{
            .session = session,
            .maxGas = maxCurrentGas,
            .request = request,
    };
    pthread_t new_thread;
    int err = pthread_create(&new_thread, nullptr, callerThread, &args);
    if (err != 0) {
        // there is no need to unblock signals.
        throw std::runtime_error("error creating new thread");
    }

    void* ret_ptr = nullptr;
    pthread_join(new_thread, &ret_ptr);

    // as soon as possible we should release the entrance lock of the app (if any)
    exit_area(session);

    int ret = *(int*) ret_ptr;
    free(ret_ptr);

    if (ret < BAD_REQUEST) {
        for (const auto& dCall: session->currentCall->deferredCalls) {
            int temp = invoke_dispatcher(
                    session,
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

void executeSession(int transactionInfo) {
    SessionInfo::CallContext newCall = {
            .appID = 0,
            .remainingExternalGas = 500000,
    };
    SessionInfo session{
            .heapModifier = unique_ptr<HeapModifier>(Heap::setupSession(transactionInfo)),
            .currentCall = &newCall,
    };
    int ret = invoke_dispatcher(&session, 50, 1, String{"Hey!", 5});
    std::cout << "returned: " << ret << std::endl;
}
