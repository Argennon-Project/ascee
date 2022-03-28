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
#include <csetjmp>

#include <string>
#include <unordered_map>
#include <vector>

#include "executor/FailureManager.h"
#include "ThreadCpuTimer.h"
#include "VirtualSignatureManager.h"
#include "AppTable.h"
#include "heap/RestrictedModifier.h"
#include "util/crypto/CryptoSystem.h"

// We can not use virtual function since runtime binding can hurt performance. The better option for us is to use
// templates. However, since the session object needs to be available in almost all argc functions it has to be defined
// static or the code will really get ugly. (argc function can not be defined as members of a class)
// Defining session as a static thread_local variable makes using templates a bit tricky. So I decided to use the
// aliasing technique instead.
#if defined(ASCEE_MOCK_BUILD)

#include "mock/heap/MockModifier_test.h"

namespace argennon::ascee::runtime { using HeapModifier = argennon::mocking::ascee::MockModifier; }

#elif defined(ASCEE_ORACLE_BUILD)

namespace argennon::ascee::runtime { using HeapModifier = OracleModifier; }

#else

#include "heap/RestrictedModifier.h"

namespace argennon::ascee::runtime { using HeapModifier = RestrictedModifier; }

#endif


namespace argennon::ascee::runtime {

struct AppRequest {
    AppRequestIdType id;
    long_id calledAppID;
    std::string httpRequest;
    int32_fast maxClocks;
    HeapModifier modifier;
    AppTable appTable;
    bool useControlledExecution;
    FailureManager failureManager;
    std::vector<AppRequestIdType> attachments;
    VirtualSignatureManager signatureManager;
    Digest digest;
};

struct AppResponse {
    int statusCode;
    std::string httpResponse;
};

struct DeferredArgs {
    long_id appID;
    std::string request;
};

class Executor {
private:
    class CallManager {
    public:
        virtual int executeApp(byte forwarded_gas, long_id app_id,
                               response_buffer_c& response, string_view_c request) = 0;

        virtual void guardArea_() = 0;

        virtual void unGuard_() = 0;

    };

    class ControlledCaller : public CallManager {
    public:
        class CallResourceHandler {
        public:
            explicit CallResourceHandler(byte forwardedGas);

            explicit CallResourceHandler(int_fast32_t initialGas);

            [[nodiscard]] int_fast64_t getExecTime() const { return session->failureManager.getExecTime(id, gas); }

            [[nodiscard]] std::size_t getStackSize() const { return session->failureManager.getStackSize(id); }

            void complete() { completed = true; }

            ~CallResourceHandler() noexcept;

        private:
            int_fast32_t id;
            long_id caller;
            int_fast32_t gas;
            int_fast32_t remainingExternalGas;
            int16_t heapVersion;
            CallResourceHandler* prevResources = nullptr;
            bool completed = false;
        };

        int executeApp(byte forwarded_gas, long_id app_id,
                       response_buffer_c& response, string_view_c request) override;

        void guardArea_() override;

        void unGuard_() override;

    private:
        static void* threadStart(void* voidArgs);
    };

    class OptimisticCaller : public CallManager {
    public:
        class CallResourceHandler {
        public:
            explicit CallResourceHandler();

            void complete() { completed = true; }

            ~CallResourceHandler() noexcept;

        private:
            long_id caller;
            int16_t heapVersion;
            bool completed = false;
        };

        int executeApp(byte forwarded_gas, long_id app_id,
                       response_buffer_c& response, string_view_c request) override;

        void guardArea_() override {}

        void unGuard_() override {}
    };

public:
    static constexpr int min_clocks = 1;
    static constexpr int64_t default_exec_time_nsec = 50000000;

    class Error : public AsceeError {
    public:
        explicit Error(
                const AsceeError& ae,
                long_id app = session->currentCall->appID
        ) noexcept: AsceeError(ae), app(app) {}

        explicit Error(
                const std::string& msg,
                StatusCode code = StatusCode::internal_error,
                const std::string& thrower = "",
                long_id app = session->currentCall->appID
        ) noexcept: AsceeError(msg, code, thrower), app(app) {}

        explicit Error(const std::string& msg, StatusCode code, long_id app) noexcept:
                AsceeError(msg, code, ""), app(app) {}

        template<int size>
        auto& toHttpResponse(runtime::StringBuffer<size>& response) const {
            response << "HTTP/1.1 " << errorCode() << " ";
            response << gReasonByStatusCode(code) << "\r\n";
            response << "Server: " << app_trie_g.toDecimalStr(app) << "\r\n";
            response << "Content-Length: " << (int) message.size() + 8 << "\r\n\r\n";
            response << "Error: " << message << ".";
            return response;
        }

        const long_id app;
    };

    class CallContext {
    public:
        CallContext();

        const long_id appID = 0;
        bool hasLock = false;
        std::vector<DeferredArgs> deferredCalls;
        CallContext* prevCallInfo = nullptr;
        jmp_buf env{};

        explicit CallContext(long_id app);

        ~CallContext() noexcept;
    };

    struct SessionInfo {
        AppRequest* request = nullptr;
        bool guardedArea = false;

        HeapModifier& heapModifier = request->modifier;
        const AppTable& appTable = request->appTable;
        FailureManager& failureManager = request->failureManager;
        std::unordered_map<uint64_t, bool> isLocked;
        VirtualSignatureManager& sigManager = request->signatureManager;
        std::unique_ptr<CallManager> callManager = nullptr;
        //util::CryptoSystem& cryptoSigner;

        CallContext* currentCall = nullptr;
        ControlledCaller::CallResourceHandler* currentResources = nullptr;

        //SessionInfo(const SessionInfo&) = delete;
    };

    /**
     * instances of Executor should not be shared between different threads.
     */
    Executor();

    inline static SessionInfo* getSession() { return session; }

    /**
     * The correct usage of this function is to use it at the start of the critical area and only call unGuard_() when the
     * code is completed normally. For example:
     * @code
     * int f() {
     *     guardArea_();
     *
     *     // no unGuard_ should be called here.
     *     throw std::exception();
     *
     *     unGuard_();
     *     return 0;
     * }
     */
    static void guardArea() { session->callManager->guardArea_(); }

    static void unGuard() { session->callManager->unGuard_(); }

    AppResponse executeOne(AppRequest* req);

    static
    int callApp(byte forwarded_gas, long_id app_id, response_buffer_c& response, string_view_c request);

private:
    // IMPORTANT: PBC library is not thread-safe, and this instance should not be shared between threads.
    //util::CryptoSystem cryptoSigner;

    static inline thread_local SessionInfo* session = nullptr;

    static inline std::atomic<bool> initialized = false;

    static void sig_handler(int sig, siginfo_t* info, void* ucontext);

    static void initHandlers();

    static void* registerRecoveryStack();
};

} // namespace argennon::ascee::runtime
#endif // ARGENNON_EXECUTOR_H
