// Copyright (c) 2022 aybehrouz <behrouz_ayati@yahoo.com>. All rights
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

#include "AppTable.h"

using namespace argennon;
using namespace ascee::runtime;

int AppTable::callApp(long_id appID, response_buffer_c& response, string_view_c request) const {
    return callTable.at(appID)(response, request);
}

void AppTable::checkApp(long_id appID) const {
    try {
        if (callTable.at(appID) == nullptr) {
            throw AsceeError("app does not exist", StatusCode::not_found);
        }
    } catch (const std::out_of_range&) {
        throw AsceeError("app/" + std::to_string(appID) + " was not declared in the call list",
                         StatusCode::limit_violated);
    }
}

AppTable::AppTable(util::OrderedStaticMap<long_id, DispatcherPointer> callTable) : callTable(std::move(callTable)) {}
