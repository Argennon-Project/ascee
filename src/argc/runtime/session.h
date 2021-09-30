
#ifndef ASCEE_SESSION_H
#define ASCEE_SESSION_H

#include <pthread.h>
#include <csetjmp>

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

#include "../../../include/argc/types.h"
#include "../../heap/HeapModifier.h"
#include "../../ThreadCpuTimer.h"

#define RESPONSE_MAX_SIZE 2*1024
namespace ascee {

struct DeferredArgs {
    std_id_t appID;
    byte forwardedGas;
    std::string request;
};

struct SessionInfo {
    jmp_buf* recentEnvPointer;
    jmp_buf* rootEnvPointer;
    bool criticalArea = false;
    std::unique_ptr<ascee::HeapModifier> heapModifier;
    ascee::ThreadCpuTimer cpuTimer;
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

// extern thread_local jmp_buf* envPointer;
extern thread_local SessionInfo* session;

} // namespace ascee

#endif // ASCEE_SESSION_H
