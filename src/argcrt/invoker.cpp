
#include <pthread.h>
#include <cstdio>
#include <ctime>
#include <csignal>
#include <cstdlib>

#include <cstring>
#include <memory>
#include <unordered_map>
#include <vector>

#include "../../include/argc/types.h"
#include "../heap/HeapModifier.h"
#include "../heap/Heap.h"
#include "../loader/AppLoader.h"

#define RESPONSE_MAX_SIZE 2*1024

using std::string, std::unique_ptr, std::vector;

struct SessionInfo;

struct DispatcherArgs {
    SessionInfo* session;
    std_id_t appID;
    string_t &request;
};

struct DeferredArgs {
    std_id_t appID;
    string request;
};

struct SessionInfo {
    unique_ptr<HeapModifier> heapModifier;
    unordered_map<std_id_t, bool> entranceLocks;
    StringBuffer responseBuffer = {
            .buffer = (char[RESPONSE_MAX_SIZE]) {},
            .maxSize = RESPONSE_MAX_SIZE,
            .end = 0
    };

    struct CallContext {
        std_id_t appID;
        int remainingExternalGas;
        vector<unique_ptr<DeferredArgs>> deferredCalls;
    };

    CallContext* currentCall;
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
    //todo: check null return
    int ret = AppLoader::getDispatcher(arguments->appID)(arguments->session, arguments->request);

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

    // we save a check point before changing the context
    session->heapModifier->saveCheckPoint();
    session->heapModifier->changeContext(app_id);

    DispatcherArgs args{
            .session = session,
            .appID = app_id,
            .request = request,
    };
    pthread_t new_thread;
    if (pthread_create(&new_thread, nullptr, callerThread, &args) != 0) {
        throw std::runtime_error("error creating new thread");
    }

    // Creating the execution timer
    timer_t exec_timer;
    struct sigevent sev{};
    struct itimerspec its{};
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGALRM;
    sev.sigev_value.sival_ptr = &new_thread;
    if (timer_create(CLOCK_THREAD_CPUTIME_ID, &sev, &exec_timer) != 0) {
        throw std::runtime_error("error in creating the cpu timer");
    }

    // Start the timer
    auto time_nsec = calculate_max_time(maxCurrentGas);
    its.it_value.tv_sec = time_nsec / 1000000000;
    its.it_value.tv_nsec = time_nsec % 1000000000;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    if (timer_settime(exec_timer, 0, &its, nullptr) != 0) {
        throw std::runtime_error("error in starting the timer");
    }

    void* ret_ptr = malloc(sizeof(int));
    pthread_join(new_thread, &ret_ptr);
    int ret = *(int*) ret_ptr;
    free(ret_ptr);

    auto &deferredCalls = session->currentCall->deferredCalls;
    if (ret < BAD_REQUEST) {
        for (auto &dCall: deferredCalls) {
            int temp = invoke_dispatcher(session, 0, dCall->appID,
                                         string_t{dCall->request.c_str(),
                                                  static_cast<int>(dCall->request.length() + 1)});
            if (temp >= BAD_REQUEST) {
                ret = temp;
                break;
            }
        }
    }

    if (ret >= BAD_REQUEST) {
        session->heapModifier->RestoreCheckPoint();
    } else {
        session->heapModifier->DiscardCheckPoint();
    }
    session->heapModifier->changeContext(oldCallInfo->appID);

    // restore previous call info
    session->currentCall = oldCallInfo;
    return ret;
}

extern "C"
int64 loadInt64(void* session, int32 offset) {
    return static_cast<SessionInfo*>(session)->heapModifier->loadInt64(offset);
}

void executeSession(int transactionInfo) {
    SessionInfo::CallContext newCall = {
            .appID = 0,
            .remainingExternalGas = 40000,
    };
    SessionInfo session{
            .heapModifier = unique_ptr<struct HeapModifier>(Heap::setupSession(transactionInfo)),
            .currentCall = &newCall,
    };
    invoke_dispatcher(&session, 50, 1, String{});
}
