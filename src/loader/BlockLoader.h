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
    enum class Type : byte {
        int_additive, read_only, writable
    };
    int32 size;
    Type accessType;
    int_fast32_t requestID;
};

using AppRequestIdType = int_fast32_t;

struct AppRequestRawData {
    using AccessMapType = util::FixedOrderedMap<long_id,
            util::FixedOrderedMap<long_id, util::FixedOrderedMap<int32, BlockAccessInfo>>>;
    /// The unique identifier of a request in a block. It must be a 32 bit integer in the interval [0,n), where n is
    /// the total number of requests of the block. Obviously any integer in the interval should be assigned to
    /// exactly one request.
    ///
    /// If the proposed execution DAG of the block has k nodes with zero in-degree the first k integers
    /// (integers in the interval [0,k)) should be assigned to nodes (requests) with a zero in-degree.
    /// If the request with id == 0 does not have zero in-degree in the proposed execution DAG the block will
    /// be considered invalid.
    AppRequestIdType id = 0;
    long_id calledAppID = -1;
    std::string httpRequest;
    int_fast32_t gas = 0;
    std::vector<long_id> appAccessList;
    std::unordered_set<int_fast32_t> stackSizeFailures;
    std::unordered_set<int_fast32_t> cpuTimeFailures;
    /// The list must be sorted.
    AccessMapType memoryAccessMap;
    /// This list should be checked to make sure no id is out of range.
    std::unordered_set<AppRequestIdType> adjList;
    std::vector<long_id> attachments;
    Digest digest;
};

struct PageAccessInfo {
    full_id id;
    bool isWritable;
};

struct ChunkSizeBounds {
    int32 sizeUpperBound;
    int32 sizeLowerBound;
};

struct BlockHeader {
    int_fast64_t blockNumber;
};

class BlockLoader {
public:
    void setCurrentBlock(const BlockHeader& b) {};

    AppRequestRawData loadRequest(AppRequestIdType id) { return {}; };

    int_fast32_t getNumOfRequests() { return 0; };

    /// This list includes:
    ///     1) The id of pages that contain at least one chunk needed for validating the block
    ///     2) The id of non-existent chunks that are accessed by at least one request.
    std::vector<PageAccessInfo> getPageAccessList() { return {}; };

    /// This list will not include all chunks. Only expandable chunks and accessed non-existent chunks should be
    /// included.
    util::FixedOrderedMap<full_id, ChunkSizeBounds> getProposedSizeBounds() { return {}; };
};

} // namespace ascee::runtime
#endif // ASCEE_BLOCK_LOADER_H
