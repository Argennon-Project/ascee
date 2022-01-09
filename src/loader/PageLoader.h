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

#ifndef ASCEE_PAGE_LOADER_H
#define ASCEE_PAGE_LOADER_H

#include <cassert>
#include "argc/primitives.h"
#include "heap/Page.h"
#include "BlockLoader.h"

namespace ascee::runtime {

class PageLoader {
public:
    void preparePage(full_id pageID, const heap::Page& page) {
        // if page.getBlockNumber() == previousBlock.blockNumber) that means we need to submit a request for getting
        // the proof of non-existence
        submitGetPageRequest(pageID, page.getBlockNumber(), previousBlock.blockNumber);
    }

    heap::Delta getDelta(full_id pageID, int_fast64_t from, int_fast64_t to, int tries) {
        return heap::Delta();
    }

    void loadPage(full_id pageID, heap::Page& page) {
        int tries = 0;
        while (true) {
            // submitGetDeltaRequest() needs to be called before.
            auto delta = getDelta(pageID, page.getBlockNumber(), previousBlock.blockNumber, tries++);
            page.applyDelta(delta, previousBlock.blockNumber);

            if (verify(pageID, page.getDigest())) break;
            else page.removeDelta(delta);
        }
    }

    void updateDigest(full_id pageID, byte* digest) {
        // page.setBlockNumber(previousBlock.blockNumber + 1);
    }

    //todo: make sure this is efficient
    void setCurrentBlock(const ascee::runtime::BlockHeader& block) {
        previousBlock = block;
    };

private:
    BlockHeader previousBlock;

    void submitGetPageRequest(full_id pageID, int_fast64_t from, int_fast64_t to) {

    }

    bool verify(full_id pageID, byte* digest) {
        return true;
    }
};

} // namespace ascee::runtime
#endif // ASCEE_PAGE_LOADER_H
