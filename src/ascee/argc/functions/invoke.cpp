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

#include <memory>
#include <unordered_map>
#include <vector>

#include <executor/Executor.h>
#include <argc/functions.h>

using namespace argennon;
using namespace ascee;
using namespace runtime;

using std::unique_ptr, std::vector, std::unordered_map, std::string, std::string_view;

void argc::enter_area() {
    if (Executor::getSession()->currentCall->hasLock) return;

    auto app = Executor::getSession()->currentCall->appID;
    if (Executor::getSession()->isLocked[app]) {
        throw Executor::GenericError("reentrancy is not allowed", StatusCode::reentrancy_attempt);
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

void argc::invoke_deferred(long_id app_id, response_buffer_c& response, string_view_c request) {
    Executor::getSession()->currentCall->deferredCalls.emplace_back(DeferredArgs{
            .appID = app_id,
            // string constructor makes a copy of its input, so we should be safe here.
            .request = std::string(request),
    });
}

void argc::revert(string_view_c msg) {
    Executor::blockSignals();
    throw Executor::GenericError(std::string(msg));
}

int argc::dependant_call(long_id app_id, response_buffer_c& response, string_view_c request) {
    try {
        Executor::getSession()->appTable.checkApp(app_id);
    } catch (const ApplicationError& err) {
        throw Executor::GenericError(err);
    }

    Executor::CallInfoContext callContext(app_id);

    Executor::unBlockSignals();
    int ret = Executor::getSession()->appTable.callApp(app_id, response, request);
    if (ret >= 400) {
        throw Executor::GenericError("returning an error code normally", StatusCode::invalid_operation);
    }
    // There is no need for blocking signals here.

    // before performing deferred calls we release the entrance lock (if any)
    argc::exit_area();

    if (!callContext.deferredCalls.empty()) {
        for (const auto& dCall: callContext.deferredCalls) {
            // Discarding the responses of deferred calls
            response_buffer_c tempBuffer;
            int temp = argc::dependant_call(dCall.appID, tempBuffer, StringView(dCall.request));
            printf("** deferred call returns: %d %s\n", temp, StringView(tempBuffer).data());
        }
    }
    return ret;
}

static
int invoke_noexcept(long_id app_id, response_buffer_c& response, string_view_c request) {
    int ret;
    try {
        try {
            ret = argc::dependant_call(app_id, response, request);
        } catch (const std::out_of_range& out) {
            throw Executor::GenericError(out.what());
        }
    } catch (const Executor::GenericError& ee) {
        Executor::blockSignals();
        ret = ee.errorCode();
        ee.toHttpResponse(response.clear());
    }
    return ret;
}

int argc::invoke_dispatcher(byte forwarded_gas, long_id app_id, response_buffer_c& response, string_view_c request) {
    Executor::blockSignals();

    int ret;
    try {
        Executor::CallResourceContext resourceContext(forwarded_gas);
        ret = Executor::controlledExec(invoke_noexcept, app_id, response, request,
                                       resourceContext.getExecTime(), resourceContext.getStackSize());
        if (ret < 400) resourceContext.complete();
    } catch (const ApplicationError& ae) {
        ret = ae.errorCode();
        Executor::GenericError(ae).toHttpResponse(response.clear());
    }

    // unBlockSignals() should be called here, in case resourceContext's constructor throws an exception.
    Executor::unBlockSignals();
    return ret;
}

