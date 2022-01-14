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

#ifndef ARGENNON_HEAP_PAGE_H
#define ARGENNON_HEAP_PAGE_H

#include "arg/primitives.h"
#include <map>
#include <vector>
#include <memory>
#include "heap/Chunk.h"

namespace argennon::asa {

class Page {
    using Chunk = ascee::runtime::Chunk;
public:
    class Delta {

    };

    explicit Page(int64_fast blockNumber) : version(blockNumber) {
        native = std::make_unique<Chunk>();
    }

    void addMigrant(full_id id, Chunk* migrant) {};

    byte* getDigest() { return nullptr; };

    int64_fast getBlockNumber() const {
        return version;
    }

    void applyDelta(Delta delta, int64_fast blockNumber) {
        version = blockNumber;
    }

    void setWritableFlag(bool writable) {
        native->setWritable(writable);
        for (const auto& pair: migrants) {
            pair.second->setWritable(writable);
        }
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

    Chunk* extractChunk(full_id id) {
        return nullptr;
    }

private:
    int64_fast version = 0;
    bool writableFlag = false;
    std::unique_ptr<Chunk> native;
    std::map<full_id, std::unique_ptr<Chunk>> migrants;
};

} // namespace argennon::asa
#endif // ARGENNON_HEAP_PAGE_H
