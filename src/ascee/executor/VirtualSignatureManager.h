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

#ifndef ARGENNON_VIRTUAL_SIG_MANAGER_H
#define ARGENNON_VIRTUAL_SIG_MANAGER_H

#include <unordered_map>
#include <unordered_set>
#include "argc/types.h"
#include "argc/StringBuffer.h"

namespace argennon::ascee::runtime {

class VirtualSignatureManager {
public:
    struct SignedMessage {
        long_id issuerAccount;
        std::string message;
    };
    static const std::size_t SIG_CONSTANT_COST = 8;
    static const std::size_t MAX_COST = 128 * 1024;

    explicit VirtualSignatureManager(std::vector<SignedMessage>&& messages);

    int32_fast sign(std::string msg, long_id issuer);

    bool verify(std::string_view msg, long_id issuer, int32_fast index);

    bool verifyAndInvalidate(std::string_view msg, long_id issuer, int32_fast index);

private:
    std::size_t cost = 0;
    std::vector<SignedMessage> messages;
};

} // namespace argennon::ascee::runtime
#endif // ARGENNON_VIRTUAL_SIG_MANAGER_H
