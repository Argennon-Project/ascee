// Copyright (c) 2021 aybehrouz <behrouz_ayati@yahoo.com>. All rights reserved.
// This file is part of the C++ implementation of the Argennon smart contract
// Execution Environment (AscEE).
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
#include <util/IdentifierTrie.h>
#include <mutex>
#include <cassert>
#include "Chunk.h"
#include "Page.h"
#include "loader/PageLoader.h"
#include "loader/BlockLoader.h"

namespace ascee::runtime::heap {

//TODO: Heap must be signal-safe but it does not need to be thread-safe
class PageCache {
public:
    class ChunkIndex {
    public:
        /// use std::move for passing requiredPages when possible.
        ChunkIndex(PageCache& parent, std::vector<PageAccessType> requiredPages, const Block& block)
                : parent(parent), requiredPages(std::move(requiredPages)) {
            chunkIndex.reserve(this->requiredPages.size() * 12 / 10);
            parent.loader.loadBlock(block);
            for (const auto& page: this->requiredPages) {
                parent.loader.preparePage(page.id, parent.cache[page.id]);
            }

            for (const auto& page: this->requiredPages) {
                indexPage(page);
            }
        }

        Chunk* getChunk(full_id id) {
            try {
                return chunkIndex.at(id);
            } catch (const std::out_of_range&) {
                throw BlockError("missing chunk");
            }
        };

        void finalize() {
            for (const auto& page: requiredPages) {
                if (page.isWritable) {
                    parent.loader.updateDigest(page.id, parent.cache.at(page.id).getDigest());
                }
            }
        };

        void migrateChunk(full_id id, full_id fromPage, full_id toPage);

    private:
        PageCache& parent;
        std::vector<PageAccessType> requiredPages;
        std::unordered_map<int128, Chunk*> chunkIndex;

        void indexPage(const PageAccessType& accessInfo) {
            auto& page = parent.cache[accessInfo.id];
            parent.loader.loadPage(accessInfo.id, page);
            chunkIndex.emplace(accessInfo.id, page.getNative(accessInfo.isWritable));
            auto chunkList = page.getMigrants(accessInfo.isWritable);
            chunkIndex.insert(chunkList.begin(), chunkList.end());
        }
    };

    explicit PageCache(PageLoader& loader);

    PageCache(const PageCache&) = delete;

private:
    std::unordered_map<int128, Page> cache;
    PageLoader& loader;
};

} // namespace ascee::runtime::heap
#endif // ASCEE_PAGE_CACHE_H
