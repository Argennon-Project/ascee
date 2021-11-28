// Copyright (c) 2021 aybehrouz <behrouz_ayati@yahoo.com>. All rights reserved.
// This file is part of the C++ implementation of the Argennon smart contract
// Execution Environment (AscEE).
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option) any
// later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License
// for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

#ifndef ASCEE_EXECUTOR_H
#define ASCEE_EXECUTOR_H


#include <pthread.h>
#include <csetjmp>
#include <csignal>

#include <string>
#include <unordered_map>
#include <memory>
#include <utility>
#include <vector>

#include <executor/FailureManager.h>
#include <argc/primitives.h>
#include <heap/Heap.h>
#include "ThreadCpuTimer.h"
#include <util/VirtualSigManager.h>

namespace ascee::runtime {

struct DeferredArgs {
    long_id appID;
    byte forwardedGas;
    std::string request;
};


struct Transaction {
    long_id calledAppID;
    string_c request;
    int_fast32_t gas;
    std::vector<long_id> appAccessList;
    FailureManager::FailureMap failedCalls;
    std::vector<AppMemAccess> memoryAccessList;
};

class Executor {
public:
    class CallInfoContext {
    public:
        const long_id appID;
        bool hasLock = false;
        std::vector<DeferredArgs> deferredCalls;
        CallInfoContext* prevCallInfo = nullptr;

        explicit CallInfoContext(long_id app);

        ~CallInfoContext() noexcept;
    };

    class CallResourceContext {
    public:
        explicit CallResourceContext(byte forwardedGas);

        explicit CallResourceContext(int_fast32_t initialGas);

        [[nodiscard]] int_fast64_t getExecTime() const { return session->failureManager.getExecTime(id, gas); }

        [[nodiscard]] std::size_t getStackSize() const { return session->failureManager.getStackSize(id); }

        void complete() { completed = true; }

        ~CallResourceContext() noexcept {
            session->currentResources = prevResources;
            session->failureManager.completeInvocation();
            if (!completed) {
                session->heapModifier.restoreVersion(heapVersion);
            }
        }

    private:
        int_fast32_t id;
        int_fast32_t gas;
        int_fast32_t remainingExternalGas;
        int16_t heapVersion;
        CallResourceContext* prevResources = nullptr;
        bool completed = false;
    };

    struct SessionInfo {
        bool criticalArea = false;

        Heap::Modifier heapModifier;
        std::unordered_map<long_id, dispatcher_ptr> appTable;
        std::unordered_map<long_id, bool> isLocked;
        FailureManager failureManager;
        VirtualSigManager virtualSigner;

        string_buffer_c<RESPONSE_MAX_SIZE> response;

        CallInfoContext* currentCall = nullptr;
        CallResourceContext* currentResources = nullptr;

        //SessionInfo(const SessionInfo&) = delete;
    };

    Executor();

    static void* registerRecoveryStack();

    inline
    static SessionInfo* getSession() { return session; }

    static void blockSignals();

    static void unBlockSignals();

    std::string startSession(const Transaction& tx);

    static
    int controlledExec(int (* invoker)(long_id, string_c), long_id app_id, string_c request,
                       int_fast64_t execTime, size_t stackSize);

private:
    static thread_local SessionInfo* session;

    Heap heap;

    static void sig_handler(int sig, siginfo_t* info, void* ucontext);

    static void initHandlers();


    static void* threadStart(void* voidArgs);
};

} // namespace ascee::runtime
#endif // ASCEE_EXECUTOR_H
