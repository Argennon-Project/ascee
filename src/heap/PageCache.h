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

namespace ascee::runtime::heap {

class PageLoader {
public:
    void preparePage(full_id pageID, const Page& page, int_fast64_t blockNumber) {
        assert(page.getBlockNumber() < blockNumber);
        submitDownloadRequest(pageID, page.getBlockNumber(), blockNumber);
    }


    Delta getDelta(full_id pageID, int_fast64_t from, int_fast64_t to, int tries) {
        return Delta();
    }

    void loadPage(full_id pageID, Page& page, int_fast64_t blockNumber) {
        int tries = 0;
        while (true) {
            auto delta = getDelta(pageID, page.getBlockNumber(), blockNumber, tries++);
            page.applyDelta(delta, blockNumber);

            if (verify(pageID, page.getDigest())) break;
            else page.removeDelta(delta);
        }
    }

    void updateDigest(full_id pageID, byte* digest) {

    }

private:
    void submitDownloadRequest(full_id pageID, int_fast64_t from, int_fast64_t to) {

    }

    bool verify(full_id pageID, byte* digest) {
        return true;
    }
};

//TODO: Heap must be signal-safe but it does not need to be thread-safe
class PageCache {
public:
    struct PageAccessType {
        full_id id;
        bool isWritable;
    };

    class BlockIndex {
    public:
        /// use std::move for passing requiredPages when possible.
        BlockIndex(PageCache& parent, std::vector<PageAccessType> requiredPages, int_fast64_t blockNumber)
                : parent(parent), requiredPages(std::move(requiredPages)), blockNumber(blockNumber) {
            chunkIndex.reserve(this->requiredPages.size() * 12 / 10);
            for (const auto& page: this->requiredPages) {
                parent.loader.preparePage(page.id, parent.cache[page.id], blockNumber);
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
        int_fast64_t blockNumber = 0;
        std::vector<PageAccessType> requiredPages;
        std::unordered_map<int128, Chunk*> chunkIndex;

        void indexPage(const PageAccessType& accessInfo) {
            auto& page = parent.cache[accessInfo.id];
            parent.loader.loadPage(accessInfo.id, page, blockNumber);
            chunkIndex.emplace(accessInfo.id, page.getNative(accessInfo.isWritable));
            auto index = page.getMigrants(accessInfo.isWritable);
            chunkIndex.insert(index.begin(), index.end());
        }
    };

    PageCache();

    PageCache(const PageCache&) = delete;

private:
    std::unordered_map<int128, Page> cache;
    PageLoader loader;
};

} // namespace ascee::runtime::heap
#endif // ASCEE_PAGE_CACHE_H
