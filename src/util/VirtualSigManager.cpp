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

#include "VirtualSigManager.h"

using namespace ascee;
using namespace runtime;
using std::string;

const std::size_t VirtualSigManager::SIG_CONSTANT_COST;
const std::size_t VirtualSigManager::MAX_COST;

void VirtualSigManager::sign(long_id appID, const StringView& msg) {
    // here the msg will be copies and saved inside messages
    bool inserted = messages[appID].emplace(msg).second;
    if (inserted) cost += msg.size() + SIG_CONSTANT_COST;
    if (cost > MAX_COST) throw std::bad_alloc();
}

bool VirtualSigManager::verify(long_id appID, const StringView& msg) {
    try {
        return messages.at(appID).count(string(msg)) > 0;
    } catch (const std::out_of_range&) {
        return false;
    }
}

bool VirtualSigManager::verifyAndInvalidate(long_id appID, const StringView& msg) {
    try {
        bool erased = messages.at(appID).erase(string(msg));
        if (erased) cost -= msg.size() + SIG_CONSTANT_COST;
        return erased;
    } catch (const std::out_of_range&) {
        return false;
    }
}
