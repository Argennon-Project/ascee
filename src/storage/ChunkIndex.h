// Copyright (c) 2022-2022 aybehrouz <behrouz_ayati@yahoo.com>. All rights
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

#ifndef ARGENNON_CHUNK_INDEX_H
#define ARGENNON_CHUNK_INDEX_H

#include <vector>
#include "arg/primitives.h"
#include "arg/info.h"
#include "Page.h"
#include "util/OrderedStaticMap.hpp"
#include "heap/Chunk.h"
#include "heap/RestrictedModifier.h"


namespace argennon::asa {

class ChunkIndex {
    using Chunk = ascee::runtime::Chunk;
public:
    ChunkIndex(
            const std::vector<std::pair<full_id, Page*>>& readonlyPages,
            std::vector<std::pair<full_id, Page*>>&& writablePages,
            util::OrderedStaticMap <full_id, ChunkBoundsInfo>&& chunkBounds,
            int32_fast numOfChunks
    );

    /// this function must be thread-safe
    Chunk* getChunk(const full_id& id);;

    int32_fast getSizeLowerBound(full_id chunkID);;

    ascee::runtime::RestrictedModifier buildModifier(const AppRequestInfo::AccessMapType& rawAccessMap);

    const std::vector<std::pair<full_id, Page*>>& getModifiedPages();

private:
    std::vector<std::pair<full_id, Page*>> writablePages;
    std::unordered_map<full_id, Chunk*, full_id::Hash> chunkIndex;

    // this map usually is small. That's why we didn't merge it with chunkIndex.
    util::OrderedStaticMap <full_id, ChunkBoundsInfo> sizeBoundsInfo;

    void indexPage(const std::pair<full_id, Page*>& pageInfo, bool writable);
};

} // namespace argennon::asa
#endif // ARGENNON_CHUNK_INDEX_H
