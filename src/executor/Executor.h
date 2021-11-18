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
#include <argc/types.h>
#include <heap/Heap.h>
#include "ThreadCpuTimer.h"

namespace ascee::runtime {

class execution_error : std::exception {
public:
    explicit execution_error(int code) : code(code) {}

    [[nodiscard]] int statusCode() const { return code; }

private:
    const int code;
};

struct DeferredArgs {
    std_id_t appID;
    byte forwardedGas;
    std::string request;
};

struct SessionInfo {
    bool criticalArea = false;

    std::unique_ptr<Heap::Modifier> heapModifier;
    std::unordered_map<std_id_t, dispatcher_ptr_t> appTable;
    std::unordered_map<std_id_t, bool> isLocked;
    FailureManager failureManager;

    string_buffer<RESPONSE_MAX_SIZE> response;

    struct CallContext {
        const std_id_t appID;
        int64_t remainingExternalGas;
        bool hasLock = false;
        std::vector<DeferredArgs> deferredCalls;
    };

    CallContext* currentCall;

    //SessionInfo(const SessionInfo&) = delete;
};

struct Transaction {
    std_id_t calledAppID;
    string_t request;
    int64_t gas;
    std::vector<std_id_t> appAccessList;
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

    static int controlledExec(int (* invoker)(byte, std_id_t, string_t),
                              byte forwarded_gas, std_id_t appID, string_t request, size_t size);
};

} // namespace ascee::runtime
#endif // ASCEE_EXECUTOR_H
