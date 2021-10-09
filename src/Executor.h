
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
#include "heap/Heap.h"
#include "ThreadCpuTimer.h"

#define RESPONSE_MAX_SIZE 2*1024

namespace ascee {

struct DeferredArgs {
    std_id_t appID;
    byte forwardedGas;
    std::string_view request;
};

struct SessionInfo {
    jmp_buf* recentEnvPointer;
    jmp_buf* rootEnvPointer;
    bool criticalArea = false;

    std::unique_ptr<ascee::Heap::Modifier> heapModifier;
    std::unordered_map<std_id_t, dispatcher_ptr_t> appTable;
    std::unordered_map<std_id_t, bool> isLocked;

    ascee::ThreadCpuTimer cpuTimer;

    char buf[RESPONSE_MAX_SIZE];
    string_buffer response = {
            .buffer = buf,
            .maxSize = RESPONSE_MAX_SIZE,
            .end = 0
    };

    struct CallContext {
        std_id_t appID;
        int64_t remainingExternalGas;
        bool hasLock = false;
        std::vector<std::unique_ptr<DeferredArgs>> deferredCalls;
    };

    CallContext* currentCall;
};

struct Transaction {
    std_id_t calledAppID;
    String request;
    int64_t gas;
    std::vector<std_id_t> appAccessList;
};

class Executor {
private:
    static thread_local SessionInfo* session;

    Heap heap;

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
