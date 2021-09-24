
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
    int maxGasUsage;
    std_id_t appID;
    string_t request;
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

int64_t calculate_max_time(int64_t max_cost) {
    return max_cost * 1000000;
}

void* caller(void* arg) {
    auto* arguments = static_cast<DispatcherArgs*>(arg);
    std_id_t calledApp = arguments->appID;

    SessionInfo::CallContext newCall = {
            .appID = calledApp,
            .remainingExternalGas = arguments->maxGasUsage / 2,
    };
    arguments->session->currentCall = &newCall;
    arguments->session->responseBuffer.end = 0;

    // we save a check point before changing the context
    arguments->session->heapModifier->saveCheckPoint();
    arguments->session->heapModifier->changeContext(calledApp);
    int ret = AppLoader::getDispatcher(calledApp)(arguments->session, arguments->request);
    if (ret >= BAD_REQUEST) {
        arguments->session->heapModifier->RestoreCheckPoint();
    } else {
        arguments->session->heapModifier->DiscardCheckPoint();
    }
    auto &deferredCalls = arguments->session->currentCall->deferredCalls;
    if (ret < BAD_REQUEST) {
        for (auto &dCall: deferredCalls) {
            DispatcherArgs dArgs = {
                    .session = arguments->session,
                    .maxGasUsage = arguments->maxGasUsage / (int) deferredCalls.size(),
                    .appID = dCall->appID,
                    .request = string_t{
                            dCall->request.c_str(),
                            static_cast<int>(dCall->request.length() + 1)
                    },
            };
            int* temp = static_cast<int*>(caller(&dArgs));
            if (*temp >= BAD_REQUEST) {
                ret = *temp;
                free(temp);
                break;
            }
            free(temp);
        }
    }
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
int invoke_dispatcher(SessionInfo* session, int max_gas, std_id_t app_id, string_t request) {
    if (session->currentCall->remainingExternalGas < max_gas) {
        return NOT_ACCEPTABLE;
    }
    session->currentCall->remainingExternalGas -= max_gas;
    auto oldCallInfo = session->currentCall;

    DispatcherArgs args{
            .session = session,
            .maxGasUsage = max_gas,
            .appID = app_id,
            .request = request,
    };
    pthread_t new_thread;
    if (pthread_create(&new_thread, nullptr, caller, &args) != 0) {
        printf("error creating thread\n");
        exit(EXIT_FAILURE);
    }

    /* Creating the execution timer */
    timer_t exec_timer;
    struct sigevent sev{};
    struct itimerspec its{};
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGALRM;
    sev.sigev_value.sival_ptr = &new_thread;
    if (timer_create(CLOCK_THREAD_CPUTIME_ID, &sev, &exec_timer) != 0) {
        printf("error in creating timer\n");
        exit(EXIT_FAILURE);
    }

    /* Start the timer */
    auto time_nsec = calculate_max_time(max_gas);
    its.it_value.tv_sec = time_nsec / 1000000000;
    its.it_value.tv_nsec = time_nsec % 1000000000;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    if (timer_settime(exec_timer, 0, &its, nullptr) != 0) {
        printf("error in starting timer\n");
        exit(EXIT_FAILURE);
    }

    // restore previous call info
    session->currentCall = oldCallInfo;

    void* ret_ptr = malloc(sizeof(int));
    pthread_join(new_thread, &ret_ptr);
    int ret = *(int*) ret_ptr;
    free(ret_ptr);
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

extern "C" inline
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
    SessionInfo session{
        .heapModifier = unique_ptr<struct HeapModifier>(heapStorage.setupSession(transactionInfo)),
    };
    invoke_dispatcher(&session, 20, 1, String{});
}
