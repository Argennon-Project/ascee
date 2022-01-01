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

#ifndef ASCEE_HEAP_PAGE_H
#define ASCEE_HEAP_PAGE_H

#include <argc/types.h>
#include <vector>
#include "Chunk.h"

namespace ascee::runtime::heap {

class Delta {

};

class Page {
public:
    void addMigrant(long_id appID, long_id chunkID, Chunk* migrant) {};

    byte* getDigest() { return nullptr; };

    int_fast64_t getBlockNumber() const {
        return blockNumber;
    }

    void applyDelta(Delta delta, int_fast64_t blockNumber) {

    }

    void removeDelta(Delta delta) {

    }

    Chunk* getNative(bool writable) {
        native.setWritable(writable);
        return &native;
    }

    std::vector<std::pair<full_id, Chunk*>> getMigrants(bool writable) {
        return std::vector<std::pair<full_id, Chunk*>>();
    }

private:
    int_fast64_t blockNumber = 0;
    Chunk native;
};

} // namespace ascee::runtime::heap
#endif // ASCEE_HEAP_PAGE_H
