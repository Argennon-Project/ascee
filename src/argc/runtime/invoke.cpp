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

#include <csignal>
#include <memory>
#include <unordered_map>
#include <vector>

#include <executor/Executor.h>
#include <argc/functions.h>


using namespace ascee;
using namespace ascee::runtime;
using std::unique_ptr, std::vector, std::unordered_map, std::string, std::string_view;

string_buffer_c<RESPONSE_MAX_SIZE>& argc::response_buffer() {
    return Executor::getSession()->response;
}

static inline
void addDefaultResponse(int statusCode) {
    char response[256];
    int n = sprintf(response, "HTTP/1.1 %d %s", statusCode, "OK");
    Executor::getSession()->response.clear();
    Executor::getSession()->response.append(string_c(response, n));
}

void argc::enter_area() {
    if (Executor::getSession()->currentCall->hasLock) return;

    auto app = Executor::getSession()->currentCall->appID;
    if (Executor::getSession()->isLocked[app]) {
        throw execution_error("reentrancy lock for " + std::to_string(app), StatusCode::reentrancy_attempt);
    } else {
        Executor::blockSignals();
        Executor::getSession()->isLocked[app] = true;
        Executor::getSession()->currentCall->hasLock = true;
        Executor::unBlockSignals();
    }
}

void argc::exit_area() {
    Executor::blockSignals();
    if (Executor::getSession()->currentCall->hasLock) {
        Executor::getSession()->isLocked[Executor::getSession()->currentCall->appID] = false;
        Executor::getSession()->currentCall->hasLock = false;
    }
    Executor::unBlockSignals();
}

void argc::invoke_deferred(long_id app_id, string_c request) {
    Executor::getSession()->currentCall->deferredCalls.emplace_back(DeferredArgs{
            .appID = app_id,
            // string constructor makes a copy of its input, so we should be safe here.
            .request = std::string(request),
    });
}

int argc::dependant_call(long_id app_id, string_c request) {
    dispatcher_ptr dispatcher;
    try {
        dispatcher = Executor::getSession()->appTable.at(app_id);
    } catch (const std::out_of_range&) {
        throw execution_error("app:" + std::to_string(app_id) + " is not declared in the call list",
                              StatusCode::limit_violated);
    }
    if (dispatcher == nullptr) {
        throw execution_error("app does not exist", StatusCode::not_found);
    }

    Executor::getSession()->response.clear();
    Executor::CallInfoContext callContext(app_id);

    Executor::unBlockSignals();
    int ret = dispatcher(request);
    if (ret >= 400) {
        throw execution_error("returning an error code normally", StatusCode::invalid_operation);
    }
    // There is no need for blocking signals here.

    // before performing deferred calls we release the entrance lock (if any)
    argc::exit_area();

    // Here we use heap memory for keeping a copy of the main response.
    string mainResponse(string_c(Executor::getSession()->response));
    for (const auto& dCall: callContext.deferredCalls) {
        int temp = argc::dependant_call(dCall.appID, string_c(dCall.request));
        printf("** deferred call returns: %d\n", temp);
    }

    // Discarding the responses of deferred calls and restoring the original response
    Executor::getSession()->response.clear();
    Executor::getSession()->response.append(StringView(mainResponse));

    return ret;
}

static
int invoke_noexcept(long_id app_id, string_c request) {
    int ret;
    try {
        try {
            ret = argc::dependant_call(app_id, request);
        } catch (const std::out_of_range& out) {
            throw execution_error(out.what());
        }
    } catch (const execution_error& ee) {
        Executor::blockSignals();
        ret = ee.errorCode();
        ee.toHttpResponse(Executor::getSession()->response.clear());
    }
    return ret;
}

int argc::invoke_dispatcher(byte forwarded_gas, long_id app_id, string_c request) {
    Executor::blockSignals();

    int ret;
    try {
        Executor::CallResourceContext resourceContext(forwarded_gas);
        ret = Executor::controlledExec(invoke_noexcept, app_id, request,
                                       resourceContext.getExecTime(), resourceContext.getStackSize());
        if (ret < 400) resourceContext.complete();
    } catch (const execution_error& ee) {
        ret = ee.errorCode();
        ee.toHttpResponse(Executor::getSession()->response.clear());
    }

    // unBlockSignals() should be called here, in case resourceContext's constructor throws an exception.
    Executor::unBlockSignals();
    return ret;
}

