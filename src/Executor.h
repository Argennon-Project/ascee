
#ifndef ASCEE_EXECUTOR_H
#define ASCEE_EXECUTOR_H


#include <pthread.h>
#include <csetjmp>
#include <csignal>

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

#include "argc/types.h"
#include "heap/HeapModifier.h"
#include "ThreadCpuTimer.h"

#define RESPONSE_MAX_SIZE 2*1024

namespace ascee {

struct DeferredArgs {
    argc::std_id_t appID;
    argc::byte forwardedGas;
    std::string_view request;
};

struct SessionInfo {
    jmp_buf* recentEnvPointer;
    jmp_buf* rootEnvPointer;
    bool criticalArea = false;

    std::unique_ptr<ascee::HeapModifier> heapModifier;
    std::unordered_map<argc::std_id_t, argc::dispatcher_ptr_t> appTable;
    std::unordered_map<argc::std_id_t, bool> isLocked;

    ascee::ThreadCpuTimer cpuTimer;

    char buf[RESPONSE_MAX_SIZE];
    argc::StringBuffer response = {
            .buffer = buf,
            .maxSize = RESPONSE_MAX_SIZE,
            .end = 0
    };

    struct CallContext {
        argc::std_id_t appID;
        int64_t remainingExternalGas;
        bool hasLock = false;
        std::vector<std::unique_ptr<DeferredArgs>> deferredCalls;
    };

    CallContext* currentCall;
};

struct Transaction {
    argc::std_id_t calledAppID;
    argc::String request;
    int64_t gas;
    std::vector<argc::std_id_t> appAccessList;
};

class Executor {
private:
    static thread_local SessionInfo* session;

    static void sig_handler(int sig, siginfo_t* info, void* ucontext);

    void* registerRecoveryStack();

    void initHandlers();

public:
    Executor();

    static SessionInfo* getSession() {
        return session;
    }

    std::string_view startSession(const Transaction& t);
};

} // namespace ascee
#endif // ASCEE_EXECUTOR_H
