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

#ifndef ASCEE_VIRTUAL_SIG_MANAGER_H
#define ASCEE_VIRTUAL_SIG_MANAGER_H

#include <unordered_map>
#include <unordered_set>
#include <argc/types.h>

namespace ascee::runtime {

class VirtualSigManager {
public:
    static const std::size_t SIG_CONSTANT_COST = 8;
    static const std::size_t MAX_COST = 128 * 1024;

    void sign(long_id appID, const StringView& msg);

    bool verify(long_id appID, const StringView& msg);

    bool verifyAndInvalidate(long_id appID, const StringView& msg);

private:
    std::size_t cost = 0;
    std::unordered_map<long_id, std::unordered_set<std::string>> messages;
};

} // namespace ascee::runtime
#endif // ASCEE_VIRTUAL_SIG_MANAGER_H
