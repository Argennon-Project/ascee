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

#include <executor/Executor.h>
#include <argc/functions.h>
#include <csetjmp>

using namespace argennon;
using namespace ascee;
using namespace runtime;


void argc::enter_area() {
    if (Executor::getSession()->currentCall->hasLock) return;

    auto app = Executor::getSession()->currentCall->appID;
    if (Executor::getSession()->isLocked[app]) {
        throw Executor::Error("reentrancy is not allowed", StatusCode::reentrancy_attempt);
    } else {
        Executor::guardArea();
        Executor::getSession()->isLocked[app] = true;
        Executor::getSession()->currentCall->hasLock = true;
        Executor::unGuard();
    }
}

void argc::exit_area() {
    Executor::guardArea();
    if (Executor::getSession()->currentCall->hasLock) {
        Executor::getSession()->isLocked[Executor::getSession()->currentCall->appID] = false;
        Executor::getSession()->currentCall->hasLock = false;
    }
    Executor::unGuard();
}

void argc::invoke_deferred(long_id app_id, response_buffer_c& response, string_view_c request) {
    Executor::getSession()->currentCall->deferredCalls.emplace_back(DeferredArgs{
            .appID = app_id,
            // string constructor makes a copy of its input, so we should be safe here.
            .request = std::string(request),
    });
}

void argc::revert(string_view_c msg) {
    Executor::guardArea();
    throw Executor::Error(std::string(msg), StatusCode::bad_request);
}

int argc::dependant_call(long_id app_id, response_buffer_c& response, string_view_c request) {
    Executor::guardArea();
    try {
        Executor::getSession()->appTable.checkApp(app_id);
    } catch (const AsceeError& err) {
        throw Executor::Error(err);
    }

    Executor::CallContext callContext(app_id);

    int jmpRet = sigsetjmp(callContext.env, true);
    Executor::guardArea();

    int ret = 0;
    if (jmpRet == 0) {
        try {
            struct UnGuarder {
                UnGuarder() { Executor::unGuard(); }

                ~UnGuarder() noexcept { Executor::guardArea(); }
            } dummy;
            ret = Executor::getSession()->appTable.callApp(app_id, response, request);
        } catch (const std::out_of_range& err) {
            throw Executor::Error(err.what(), StatusCode::out_of_range);
        }
    }

    if (ret >= 400) {
        throw Executor::Error("returning a revert code normally", StatusCode::invalid_operation);
    }

    switch (StatusCode(jmpRet)) {
        case StatusCode::memory_fault:
            throw Executor::Error("segmentation fault (possibly stack overflow)", StatusCode::memory_fault);
        case StatusCode::execution_timeout:
            throw Executor::Error("cpu timer expired", StatusCode::execution_timeout);
        case StatusCode::arithmetic_error:
            throw Executor::Error("SIGFPE was caught", StatusCode::arithmetic_error);
        default:
            if (jmpRet != 0) throw Executor::Error("a signal was caught");
    }

    // before performing deferred calls we release the entrance lock (if any)
    argc::exit_area();
    if (!callContext.deferredCalls.empty()) {
        for (const auto& dCall: callContext.deferredCalls) {
            // Discarding the responses of deferred calls
            response_buffer_c tempBuffer;
            argc::dependant_call(dCall.appID, tempBuffer, StringView(dCall.request));
        }
    }
    Executor::unGuard();
    return ret;
}


int argc::invoke_dispatcher(byte forwarded_gas, long_id app_id, response_buffer_c& response, string_view_c request) {
    return Executor::callApp(forwarded_gas, app_id, response, request);
}

