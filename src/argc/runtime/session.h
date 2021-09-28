
#ifndef ASCEE_SESSION_H
#define ASCEE_SESSION_H


#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <pthread.h>
#include "../../../include/argc/types.h"
#include "../../heap/HeapModifier.h"

#define RESPONSE_MAX_SIZE 2*1024
#define THREAD_EXIT(ret_val)   int* ret = (int*) malloc(sizeof(int)); *ret = (ret_val); pthread_exit(ret)

struct DeferredArgs {
    std_id_t appID;
    byte forwardedGas;
    std::string request;
};

struct SessionInfo {
    std::unique_ptr<ascee::HeapModifier> heapModifier;
    std::unordered_map<std_id_t, bool> isLocked;
    StringBuffer responseBuffer = {
            .buffer = (char[RESPONSE_MAX_SIZE]) {},
            .maxSize = RESPONSE_MAX_SIZE,
            .end = 0
    };

    struct CallContext {
        std_id_t appID;
        int remainingExternalGas;
        bool hasLock = false;
        std::vector<std::unique_ptr<DeferredArgs>> deferredCalls;
    };

    CallContext* currentCall;
};

#endif // ASCEE_SESSION_H
