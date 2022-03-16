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

#include <utility>
#include <vector>
#include "VirtualSignatureManager.h"

using namespace argennon;
using namespace ascee::runtime;

using std::string;

const std::size_t VirtualSignatureManager::SIG_CONSTANT_COST;
const std::size_t VirtualSignatureManager::MAX_COST;

int32_fast VirtualSignatureManager::sign(std::string msg, long_id issuer) {
    // here the msg will be copies and saved inside messages
    cost += msg.size() + SIG_CONSTANT_COST;
    if (cost > MAX_COST) throw AsceeError("too many virtual signatures", StatusCode::limit_exceeded);
    messages.emplace_back(SignedMessage{issuer, std::move(msg)});
    return int32_fast(messages.size()) - 1;
}

bool VirtualSignatureManager::verifyAndInvalidate(std::string_view msg, long_id issuer, int32_fast index) {
    if (verify(msg, issuer, index)) {
        messages[index].message = "";
        cost -= msg.size() + SIG_CONSTANT_COST;
        return true;
    }
    return false;
}

VirtualSignatureManager::VirtualSignatureManager(
        std::vector<SignedMessage>&& messages
) : messages(std::move(messages)) {}

bool VirtualSignatureManager::verify(std::string_view msg, long_id issuer, int32_fast index) {
    try {
        const auto& sig = messages.at(index);
        return sig.message == msg && sig.issuerAccount == issuer;
    } catch (const std::out_of_range&) {
        return false;
    }
}
