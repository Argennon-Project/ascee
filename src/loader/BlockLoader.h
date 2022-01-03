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

#ifndef ASCEE_BLOCK_LOADER_H
#define ASCEE_BLOCK_LOADER_H

#include <vector>
#include <string>
#include <unordered_set>
#include "argc/primitives.h"
#include "util/FixedOrderedMap.hpp"


namespace ascee::runtime {

struct BlockAccessInfo {
    int32 size;
    bool writable;
    int_fast32_t requestID;
};

using AppRequestIdType = int_fast32_t;

struct AppRequestRawData {
    using AccessMapType = util::FixedOrderedMap<long_id,
            util::FixedOrderedMap<long_id, util::FixedOrderedMap<int32, BlockAccessInfo>>>;
    AppRequestIdType id = 0;
    long_id calledAppID = -1;
    std::string httpRequest;
    int_fast32_t gas = 0;
    std::vector<long_id> appAccessList;
    std::unordered_set<int_fast32_t> stackSizeFailures;
    std::unordered_set<int_fast32_t> cpuTimeFailures;
    /// it must check that the list is sorted
    AccessMapType memoryAccessMap;
    /// this list should be checked to make sure no id is out of range.
    std::unordered_set<AppRequestIdType> adjList;
    std::vector<long_id> attachments;
    /// this list will not include all chunks. Only expandable chunks and accessed non-existent chunks should be
    /// included.
    std::vector<std::pair<long_id, int32>> chunkCapacities;
    Digest digest;
};

struct PageAccessInfo {
    full_id id;
    bool isWritable;
};

struct BlockHeader {
    int_fast64_t blockNumber;
};

class BlockLoader {
public:
    void setCurrentBlock(const BlockHeader& b) {};

    AppRequestRawData loadRequest(AppRequestIdType id) { return AppRequestRawData(); };

    int_fast32_t getNumOfRequests() { return 0; };

    std::vector<PageAccessInfo> getPageAccessList() { return std::vector<PageAccessInfo>(); };
};

} // namespace ascee::runtime
#endif // ASCEE_BLOCK_LOADER_H
