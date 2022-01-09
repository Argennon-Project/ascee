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
    /**
     *  The unique identifier of a request in a block. It must be a 32 bit integer in the interval [0,n), where n is
     *  the total number of requests of the block. Obviously any integer in the interval should be assigned to
     *  exactly one request.
     *
     *  If the proposed execution DAG of the block has k nodes with zero in-degree the first k integers
     *  (integers in the interval [0,k)) must be assigned to nodes (requests) with a zero in-degree, otherwise
     *  the block will be considered invalid.
     */
    // BlockLoaders must ensure: id >= 0 && id < numOfRequests
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
    /**
     *  attachments is a list of requests of the current block that are "attached" to this request. That means, for
     *  validating this request a validator first needs to "inject" the digest of attached requests into the httpRequest
     *  field of this request.
     *
     *  The main usage of this feature is for fee payment. A request that wants to pay the fees for some requests
     *  must declare those requests as its attachments. For paying fees the payer request signs the digest of requests
     *  for which it wants to pay fees for. By injecting digest of those request by validators that signature can
     *  be validated correctly by the ARG application.
     */
    // BlockLoader needs to verify that all integers in the list are in [0, numOfRequests)
    std::vector<AppRequestIdType> attachments;

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

    // BlockLoader must verify this value.
    int32_fast getNumOfRequests() { return 0; };

    /// This list includes:
    ///     1) The id of pages that contain at least one chunk needed for validating the block
    ///     2) The id of non-existent chunks that are accessed by at least one request.
    std::vector<PageAccessInfo> getPageAccessList() { return {}; };

    /// This list will not include all chunks. Only expandable chunks and accessed non-existent chunks should be
    /// included.
    util::FixedOrderedMap<full_id, ChunkSizeBounds> getProposedSizeBounds() { return {}; };

    // BlockLoader does NOT need to verify this value. (only affects performance)
    int32_fast getNumOfChunks() {
        return 0;
    }
};

} // namespace ascee::runtime
#endif // ASCEE_BLOCK_LOADER_H
