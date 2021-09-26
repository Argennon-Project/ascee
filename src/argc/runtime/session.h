
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

struct DeferredArgs {
    std_id_t appID;
    std::string request;
};

struct SessionInfo {
    std::unique_ptr<ascee::HeapModifier> heapModifier;
    std::unordered_map<std_id_t, bool> entranceLocks;
    StringBuffer responseBuffer = {
            .buffer = (char[RESPONSE_MAX_SIZE]) {},
            .maxSize = RESPONSE_MAX_SIZE,
            .end = 0
    };

    struct CallContext {
        std_id_t appID;
        int remainingExternalGas;
        std::vector<std::unique_ptr<DeferredArgs>> deferredCalls;
    };

    CallContext* currentCall;
};

struct ContextInfo {
    ascee::HeapModifier* modifier;
    std_id_t currentApp;
    std_id_t previousApp;
    pthread_t execThread;
};

#endif // ASCEE_SESSION_H
