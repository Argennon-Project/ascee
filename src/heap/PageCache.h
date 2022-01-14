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

#ifndef ASCEE_PAGE_CACHE_H
#define ASCEE_PAGE_CACHE_H


#include <argc/types.h>
#include <unordered_map>
#include <vector>
#include <util/PrefixTrie.hpp>
#include <mutex>
#include <cassert>
#include "Chunk.h"
#include "Page.h"
#include "loader/PageLoader.h"
#include "loader/BlockLoader.h"
#include "Modifier.h"

namespace ascee::runtime::heap {

//TODO: Heap must be signal-safe but it does not need to be thread-safe
class PageCache {
public:
    explicit PageCache(PageLoader& loader);

    PageCache(const PageCache&) = delete;

    std::vector<std::pair<full_id, Page*>>
    prepareBlockPages(
            const BlockHeader& block,
            const std::vector<PageAccessInfo>& pageAccessList,
            const std::vector<MigrationInfo>& chunkMigrations
    );

private:
    std::unordered_map<int128, Page> cache;
    PageLoader& loader;
};

} // namespace ascee::runtime::heap
#endif // ASCEE_PAGE_CACHE_H
