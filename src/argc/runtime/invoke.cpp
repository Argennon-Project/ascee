
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
    std_id_t previousAppID;
    int maxGas;
    string_t& request;
};

extern "C" inline
string_buffer* getResponse(SessionInfo* session) {
    return &session->responseBuffer;
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
    ContextInfo context = {
            .modifier = arguments->session->heapModifier.get(),
            .currentApp = arguments->session->currentCall->appID,
            .previousApp = arguments->previousAppID,
            .execThread = pthread_self(),
    };
    sev.sigev_value.sival_ptr = &context;
    if (timer_create(CLOCK_THREAD_CPUTIME_ID, &sev, &exec_timer) != 0) {
        throw std::runtime_error("error in creating the cpu timer");
    }

    // Start the timer
    auto time_nsec = calculate_max_time(arguments->maxGas);
    printf("%ld", time_nsec);
    its.it_value.tv_sec = time_nsec / 1000000000;
    its.it_value.tv_nsec = time_nsec % 1000000000;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    if (timer_settime(exec_timer, 0, &its, nullptr) != 0) {
        throw std::runtime_error("error in starting the timer");
    }

    auto dispatcher = AppLoader::getDispatcher(arguments->session->currentCall->appID);
    int ret = (dispatcher == nullptr) ? NOT_FOUND : dispatcher(arguments->session, arguments->request);

    // we don't let an app manually return REQUEST_TIMEOUT
    if (ret == REQUEST_TIMEOUT) ret = INTERNAL_ERROR;

    int* retMem = (int*) malloc(sizeof(int));
    *retMem = ret;
    return retMem;
}

extern "C"
void invoke_deferred(SessionInfo* session, std_id_t app_id, string_t request) {
    session->currentCall->deferredCalls.push_back(
            std::make_unique<DeferredArgs>(DeferredArgs{
                    .appID = app_id,
                    .request = string(request.content, request.length),
            }));
}

extern "C"
int invoke_dispatcher(SessionInfo* session, byte forwarded_gas, std_id_t app_id, string_t request) {
    int maxCurrentGas = (session->currentCall->remainingExternalGas * forwarded_gas) >> 8;
    session->currentCall->remainingExternalGas -= maxCurrentGas;

    auto oldCallInfo = session->currentCall;
    SessionInfo::CallContext newCall = {
            .appID = app_id,
            .remainingExternalGas = maxCurrentGas / 2,
    };
    session->currentCall = &newCall;
    session->responseBuffer.end = 0;

    DispatcherArgs args{
            .session = session,
            .previousAppID = oldCallInfo->appID,
            .maxGas = maxCurrentGas,
            .request = request,
    };

    session->heapModifier->openContext(app_id);

    pthread_t new_thread;
    if (pthread_create(&new_thread, nullptr, callerThread, &args) != 0) {
        throw std::runtime_error("error creating new thread");
    }

    void* ret_ptr = malloc(sizeof(int));
    pthread_join(new_thread, &ret_ptr);
    int ret = *(int*) ret_ptr;
    free(ret_ptr);

    auto& deferredCalls = session->currentCall->deferredCalls;
    if (ret < BAD_REQUEST) {
        for (auto& dCall: deferredCalls) {
            int temp = invoke_dispatcher(session, 0, dCall->appID,
                                         string_t{dCall->request.c_str(),
                                                  static_cast<int>(dCall->request.length() + 1)});
            if (temp >= BAD_REQUEST) {
                ret = temp;
                break;
            }
        }
    }

    // if ret == REQUEST_TIMEOUT the signal handler has already closed the context and we should not close it again.
    if (ret >= BAD_REQUEST && ret != REQUEST_TIMEOUT) {
        session->heapModifier->closeContextAbruptly(app_id, oldCallInfo->appID);
    } else {
        session->heapModifier->closeContextNormally(app_id, oldCallInfo->appID);
    }

    // restore previous call info
    session->currentCall = oldCallInfo;
    return ret;
}

void executeSession(int transactionInfo) {
    SessionInfo::CallContext newCall = {
            .appID = 0,
            .remainingExternalGas = 30000,
    };
    SessionInfo session{
            .heapModifier = unique_ptr<struct HeapModifier>(Heap::setupSession(transactionInfo)),
            .currentCall = &newCall,
    };
    invoke_dispatcher(&session, 50, 1, String{"Hey!", 5});
}
