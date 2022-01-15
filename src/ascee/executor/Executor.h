// Copyright (c) 2021-2022 aybehrouz <behrouz_ayati@yahoo.com>. All rights
// reserved. This file is part of the C++ implementation of the Argennon smart
// contract Execution Environment (AscEE).
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

#ifndef ARGENNON_EXECUTOR_H
#define ARGENNON_EXECUTOR_H

#include <csignal>

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

#include <executor/FailureManager.h>
#include <arg/primitives.h>
#include "ThreadCpuTimer.h"
#include "heap/HeapModifier.h"
#include "VirtualSigManager.h"
#include "AppTable.h"

namespace argennon::ascee::runtime {

struct AppRequest {
    AppRequestIdType id;
    long_id calledAppID;
    std::string httpRequest;
    int32_fast gas;
    HeapModifier modifier;
    AppTable appTable;
    FailureManager failureManager;
    std::vector<AppRequestIdType> attachments;
    Digest digest;
};

struct AppResponse {
    AppRequestIdType reqID;
    int statusCode;
    std::string response;
};

struct DeferredArgs {
    long_id appID;
    std::string request;
};

class Executor {
public:
    class GenericError : public ApplicationError {
    public:
        explicit GenericError(
                const ApplicationError& ae,
                long_id app = session->currentCall->appID
        ) : ApplicationError(ae), app(app) {}

        explicit GenericError(
                const std::string& msg,
                StatusCode code = StatusCode::internal_error,
                const std::string& thrower = "",
                long_id app = session->currentCall->appID
        ) noexcept: ApplicationError(msg, code, thrower), app(app) {}

        explicit GenericError(const std::string& msg, StatusCode code, long_id app) noexcept:
                ApplicationError(msg, code, ""), app(app) {}

        template<int size>
        auto& toHttpResponse(runtime::StringBuffer<size>& response) const {
            response << "HTTP/1.1 " << errorCode() << " ";
            response << gReasonByStatusCode(code) << "\r\n";
            response << "Server: " << app << "\r\n";
            response << "Content-Length: " << (int) message.size() + 8 << "\r\n\r\n";
            response << "Error: " << StringView(message) << ".";
            return response;
        }

        const long_id app;
    };

    class CallInfoContext {
    public:
        CallInfoContext();

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

        ~CallResourceContext() noexcept;

    private:
        int_fast32_t id;
        long_id caller;
        int_fast32_t gas;
        int_fast32_t remainingExternalGas;
        int16_t heapVersion;
        CallResourceContext* prevResources = nullptr;
        bool completed = false;
    };

    struct SessionInfo {
        AppRequest* request = nullptr;
        bool criticalArea = false;

        HeapModifier& heapModifier = request->modifier;
        const AppTable& appTable = request->appTable;
        FailureManager& failureManager = request->failureManager;
        std::unordered_map<long_id, bool> isLocked;
        VirtualSigManager virtualSigner;

        string_buffer_c<RESPONSE_MAX_SIZE> httpResponse;

        CallInfoContext* currentCall = nullptr;
        CallResourceContext* currentResources = nullptr;

        //SessionInfo(const SessionInfo&) = delete;
    };

    Executor();

    inline static SessionInfo* getSession() { return session; }

    static void blockSignals();

    static void unBlockSignals();

    AppResponse executeOne(AppRequest* req);

    static
    int controlledExec(int (* invoker)(long_id, string_c), long_id app_id, string_c request,
                       int_fast64_t execTime, size_t stackSize);

private:
    static void* registerRecoveryStack();

    static thread_local SessionInfo* session;

    static void sig_handler(int sig, siginfo_t* info, void* ucontext);

    static void initHandlers();

    static void* threadStart(void* voidArgs);
};

} // namespace argennon::ascee::runtime
#endif // ARGENNON_EXECUTOR_H