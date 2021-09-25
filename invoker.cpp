
#include "argctypes.h"
#include "HeapModifier.h"
#include "Heap.h"
#include "AppLoader.h"
#include <memory>
#include <unordered_map>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <cstring>
#include <memory>
#include <vector>

#define RESPONSE_MAX_SIZE 2*1024
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
    int ret = AppLoader::getDispatcher(arguments->appID)(arguments->session, arguments->request);

    int* retMem = (int*) malloc(sizeof(int));
    *retMem = ret;
    return retMem;
}

extern "C"
void invoke_deferred(SessionInfo* session, std_id_t app_id, string_t request) {
    session->currentCall->deferredCalls.push_back(
            make_unique<DeferredArgs>(DeferredArgs{
                    .appID = app_id,
                    .request = string(request.content, request.length),
            }));
}

using namespace std;
Heap heapStorage;

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
        throw runtime_error("error creating new thread");
    }

    // Creating the execution timer
    timer_t exec_timer;
    struct sigevent sev{};
    struct itimerspec its{};
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGALRM;
    sev.sigev_value.sival_ptr = &new_thread;
    if (timer_create(CLOCK_THREAD_CPUTIME_ID, &sev, &exec_timer) != 0) {
        throw runtime_error("error in creating the cpu timer");
    }

    // Start the timer
    auto time_nsec = calculate_max_time(maxCurrentGas);
    its.it_value.tv_sec = time_nsec / 1000000000;
    its.it_value.tv_nsec = time_nsec % 1000000000;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    if (timer_settime(exec_timer, 0, &its, nullptr) != 0) {
        throw runtime_error("error in starting the timer");
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

extern "C"
void append_str(StringBuffer* buf, String str) {
    if (buf->maxSize < buf->end + str.length) {
        raise(SIGSEGV);
    }
    strncpy(buf->buffer + buf->end, str.content, str.length);
    if (str.content[str.length - 1] == '\0')
        buf->end += str.length - 1;
    else
        buf->end += str.length;
}

extern "C"
void append_int64(StringBuffer* buf, int64 i) {
    string str = to_string(i);
    append_str(buf, String{str.c_str(), static_cast<int>(str.size() + 1)});
}

extern "C" inline
String buf_to_string(const StringBuffer* buf) {
    return String{
            .content = buf->buffer,
            .length = buf->end
    };
}

void executeSession(int transactionInfo) {
    SessionInfo::CallContext newCall = {
            .appID = 0,
            .remainingExternalGas = 40000,
    };
    SessionInfo session{
            .heapModifier = unique_ptr<struct HeapModifier>(heapStorage.setupSession(transactionInfo)),
            .currentCall = &newCall,
    };
    invoke_dispatcher(&session, 50, 1, String{});
}
