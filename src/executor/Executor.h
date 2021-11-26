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
#include <vector>

#include <executor/FailureManager.h>
#include <argc/primitives.h>
#include <heap/Heap.h>
#include "ThreadCpuTimer.h"
#include <util/VirtualSigManager.h>

namespace ascee::runtime {

class execution_error : std::exception {
public:
    explicit execution_error(int code) : code(code) {}

    [[nodiscard]] int statusCode() const { return code; }

private:
    const int code;
};

struct DeferredArgs {
    long_id appID;
    byte forwardedGas;
    std::string request;
};

struct SessionInfo {
    bool criticalArea = false;

    Heap::Modifier heapModifier;
    std::unordered_map<long_id, dispatcher_ptr> appTable;
    std::unordered_map<long_id, bool> isLocked;
    FailureManager failureManager;
    VirtualSigManager virtualSigner;

    string_buffer_c<RESPONSE_MAX_SIZE> response;

    struct CallContext {
        const long_id appID;
        int_fast32_t remainingExternalGas;
        bool hasLock = false;
        std::vector<DeferredArgs> deferredCalls;
    };

    CallContext* currentCall;

    //SessionInfo(const SessionInfo&) = delete;
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
    static thread_local SessionInfo* session;
private:

    Heap heap;

    static void sig_handler(int sig, siginfo_t* info, void* ucontext);

    void initHandlers();

public:
    Executor();

    static void* registerRecoveryStack();

    inline
    static SessionInfo* getSession() { return session; }

    std::string startSession(const Transaction& t);

    static int controlledExec(int (* invoker)(byte, long_id, string_c),
                              byte forwarded_gas, long_id appID, string_c request, size_t size);
};

} // namespace ascee::runtime
#endif // ASCEE_EXECUTOR_H
