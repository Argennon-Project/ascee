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

#ifndef ARGENNON_BLOCK_LOADER_H
#define ARGENNON_BLOCK_LOADER_H

#include <vector>
#include <string>
#include <unordered_set>

#include "util/OrderedStaticMap.hpp"
#include "arg/info.h"


namespace argennon::ave {

class BlockLoader {
public:
    class RequestStream {
    public:
        class EndOfStream : std::exception {
        };

        AppRequestInfo next() {
            return {};
        }
    };

    std::vector<RequestStream> createRequestStreams(int count) {
        return {};
    }

    void setCurrentBlock(const BlockInfo& b) {};

    AppRequestInfo loadRequest(AppRequestIdType id) { return {}; };

    // BlockLoader must verify this value.
    int32_fast getNumOfRequests() { return 0; };

    /// This list includes:
    ///     1) The id of pages that contain at least one chunk needed for validating the block
    ///     2) The id of non-existent chunks that are accessed by at least one request.
    std::vector<VarLenFullID> getReadonlyPageList() { return {}; };

    std::vector<VarLenFullID> getWritablePageList() { return {}; };

    /// This list will not include all chunks. Only expandable chunks and accessed non-existent chunks should be
    /// included.
    util::OrderedStaticMap<full_id, ChunkBoundsInfo> getProposedSizeBounds() { return {}; };

    // BlockLoader does NOT need to verify this value. (only affects performance)
    int32_fast getNumOfChunks() {
        return 0;
    }

    std::vector<MigrationInfo> getMigrationList() { return {}; };

    util::Digest getResponseListDigest() {
        return {};
    }
};

} // namespace argennon::ave
#endif // ARGENNON_BLOCK_LOADER_H
