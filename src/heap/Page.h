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
#include <map>
#include <vector>
#include "Chunk.h"

namespace ascee::runtime::heap {

class Delta {

};

class Page {
public:
    explicit Page(int64_fast blockNumber) : version(blockNumber) {
        native = std::make_unique<Chunk>();
    }

    void addMigrant(long_id appID, long_id chunkID, Chunk* migrant) {};

    byte* getDigest() { return nullptr; };

    int64_fast getBlockNumber() const {
        return version;
    }

    void applyDelta(Delta delta, int64_fast blockNumber) {
        version = blockNumber;
    }

    void removeDelta(Delta delta) {

    }

    [[nodiscard]]
    Chunk* getNative() {
        return native.get();
    }

    [[nodiscard]]
    const std::map<full_id, std::unique_ptr<Chunk>>& getMigrants() {
        return migrants;
    }

private:
    int64_fast version = 0;
    std::unique_ptr<Chunk> native;
    std::map<full_id, std::unique_ptr<Chunk>> migrants;
};

} // namespace ascee::runtime::heap
#endif // ASCEE_HEAP_PAGE_H
