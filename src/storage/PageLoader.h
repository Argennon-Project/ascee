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

#ifndef ARGENNON_PAGE_LOADER_H
#define ARGENNON_PAGE_LOADER_H


#include "core/primitives.h"
#include "Page.h"
#include "core/info.h"

namespace argennon::asa {

class PageLoader {
public:
    void preparePage(full_id pageID, const Page& page) {
        // if page.getBlockNumber() == previousBlock.blockNumber) that means we need to submit a request for getting
        // the proof of non-existence
        submitGetPageRequest(pageID, page.getBlockNumber(), previousBlock.blockNumber);
    }

    Page::Delta getDelta(const VarLenFullID& pageID, int_fast64_t from, int_fast64_t to, int tries) {
        return Page::Delta();
    }

    void updatePage(const VarLenFullID& pageID, Page& page) {
        int tries = 0;
        while (true) {
            // submitGetDeltaRequest() needs to be called before.
            auto delta = getDelta(pageID, page.getBlockNumber(), previousBlock.blockNumber, tries++);
            try {
                page.applyDelta(pageID, delta, previousBlock.blockNumber);
                break;
            } catch (const std::invalid_argument& err) {
                // remove invalid page from cache
            }
        }
    }

    void updateDigest(full_id pageID, byte* digest) {
        // page.setBlockNumber(previousBlock.blockNumber + 1);
    }

    //todo: make sure this is efficient
    void setCurrentBlock(const BlockInfo& block) {
        previousBlock = block;
    };

private:
    BlockInfo previousBlock;

    void submitGetPageRequest(full_id pageID, int_fast64_t from, int_fast64_t to) {

    }

    bool verify(full_id pageID, byte* digest) {
        return true;
    }
};

} // namespace argennon::asa
#endif // ARGENNON_PAGE_LOADER_H
