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

#ifndef ARGENNON_PAGE_CACHE_H
#define ARGENNON_PAGE_CACHE_H


#include <unordered_map>
#include <vector>

#include "arg/info.h"
#include "Page.h"
#include "PageLoader.h"

namespace argennon::asa {

//TODO: Heap must be signal-safe but it does not need to be thread-safe
class PageCache {
public:
    explicit PageCache(PageLoader& loader);

    PageCache(const PageCache&) = delete;

    std::vector<std::pair<full_id, Page*>>
    prepareBlockPages(
            const BlockInfo& block,
            const std::vector<PageAccessInfo>& pageAccessList,
            const std::vector<MigrationInfo>& chunkMigrations
    );

private:
    std::unordered_map<int128, Page> cache;
    PageLoader& loader;
};

} // namespace argennon::asa
#endif // ARGENNON_PAGE_CACHE_H