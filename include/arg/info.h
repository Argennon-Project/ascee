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

#ifndef ARGENNON_INFO_H
#define ARGENNON_INFO_H

#include <string>
#include <vector>
#include <unordered_set>
#include "primitives.h"
#include "id.h"
#include "util/OrderedStaticMap.hpp"

namespace argennon {

using AppRequestIdType = int32_fast;

/**
 *
 */
struct AccessBlockInfo {
    class Access {
    public:
        enum class Type : byte {
            check_only, int_additive, read_only, writable
        };

        enum class Operation : byte {
            check, int_add, read, write
        };

        Access(Type type) : type(type) {} // NOLINT(google-explicit-constructor)

        [[nodiscard]]
        bool isAdditive() const {
            return type == Type::int_additive;
        }

        bool operator==(Access other) const {
            return type == other.type;
        }

        //Access& operator=(Access& other) = default;

        [[nodiscard]]
        bool mayWrite() const {
            return type != Type::read_only && type != Type::check_only;
        }

        [[nodiscard]]
        bool denies(Operation op) const {
            switch (type) {
                case Type::check_only:
                    return op != Operation::check;
                case Type::int_additive:
                    return !(op == Operation::check || op == Operation::int_add);
                case Type::read_only:
                    return !(op == Operation::check || op == Operation::read);
                case Type::writable:
                    // int_add should not be allowed because of how it is implemented.
                    return !(op == Operation::write || op == Operation::check || op == Operation::read);
            }
            return true;
        }

        [[nodiscard]]
        bool merges(Access other) const {
            return !isAdditive() && type == other.type;
        }

        [[nodiscard]]
        bool collides(Access other) const {
            switch (type) {
                case Type::check_only:
                    return false;
                case Type::int_additive:
                    return !(other.type == Type::check_only);
                case Type::read_only:
                    return !(other.type == Type::check_only || other.type == Type::read_only);
                case Type::writable:
                    return !(other.type == Type::check_only);
            }
            return true;
        }

    private:
        Type type;
    };

    bool operator<(const AccessBlockInfo& other) const {
        return requestID < other.requestID;
    }

    int32 size;
    Access accessType;
    AppRequestIdType requestID;
};

struct AppRequestInfo {
    using AccessMapType = util::OrderedStaticMap<long_id,
            util::OrderedStaticMap<long_long_id, util::OrderedStaticMap<int32, AccessBlockInfo>>>;
    /**
     *  The unique identifier of a request in a block. It must be a 32 bit integer in the interval [0,n), where n is
     *  the total number of requests of the block. Obviously any integer in the interval should be assigned to
     *  exactly one request.
     *
     *  If the proposed execution DAG of the block has k nodes with zero in-degree the first k integers
     *  (integers in the interval [0,k)) must be assigned to nodes (requests) with zero in-degree, otherwise
     *  the block will be considered invalid.
     *
     *  For using cluster-product collision detection algorithm the order of request identifiers must
     *  respect a topological sort of the execution dag. That means in the proposed
     *  execution dag a request can not be adjacent with a request with a smaller identifier.
     */
    // BlockLoaders must ensure: id >= 0 && id < numOfRequests
    AppRequestIdType id = 0;

    long_id calledAppID = -1;
    std::string httpRequest;
    int_fast32_t gas = 0;

    std::vector<long_id> appAccessList;
    std::unordered_set<int_fast32_t> stackSizeFailures;
    std::unordered_set<int_fast32_t> cpuTimeFailures;
    /**
     * memoryAccessMap is the sorted list of memory locations that the request will access. This list must be sorted
     * based on appIDs, chunkIDs and offsets. Defined access blocks must be non-overlapping. The first defined
     * access block for every chunk MUST be ONE of the following access blocks:
     *
     *
     * {offset = -3, size = *, access = *} which means the request does not access the size of the chunk.
     *
     * {offset = -2, size = *, access = read_only} which means the request reads the chunkSize but will not modify it.
     *
     * {offset = -1, size, access = writable}, which means the request may resize the chunk. If size > 0 the request wants to
     * expand the chunk and sizeBound = size, which means newSize <= size. If size <= 0 the request can shrink the
     * chunk and sizeBound = -size, which means newSize >= -size.
     *
     * @note * indicates any value.
    */
    // block loader needs to endure that this list is: 1) sorted. 2) all AccessBlocks are non-overlapping.
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
    std::vector<AppRequestIdType> attachments;
    // BlockLoader needs to verify that all integers in the list are in [0, numOfRequests)

    util::Digest digest;
};

struct MigrationInfo {
    int32_fast chunkIndex;
    /// The index of the page that the chunk should be migrated from. Here, index means the sequence number of the
    /// page in the PageAccessInfo list. The sequence numbers starts from zero.
    int32_fast fromIndex;
    int32_fast toIndex;
};

struct ChunkBoundsInfo {
    int32 sizeUpperBound;
    int32 sizeLowerBound;
};

struct BlockInfo {
    int_fast64_t blockNumber;
};

class BlockError : public std::exception {
public:
    const std::string message;

    explicit BlockError(std::string message) : message(std::move(message)) {}

    [[nodiscard]] const char* what() const noexcept override { return message.c_str(); }
};

} // namespace argennon
#endif // ARGENNON_INFO_H
