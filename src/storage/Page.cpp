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


#include "Page.h"

using namespace argennon;
using namespace asa;
using std::pair, std::unique_ptr;

void Page::applyDelta(const VarLenFullID& pageID, const Page::Delta& delta, int64_fast blockNumber) {
    if (delta.content.empty()) return;
    const byte* boundary = delta.content.data() + delta.content.size();
    const byte* reader = delta.content.data();
    // we need this to make sure first migrant has zero index
    int32_fast index = -1;
    while (auto indexDiff = var_size_trie_g.decodeVarUInt(&reader, boundary)) {
        index += indexDiff;
        if (index == migrants.size()) {
            migrants.emplace_back(VarLenFullID(&reader, boundary));
        } else {
            migrants.at(index).id = VarLenFullID(&reader, boundary);
        }
    }

    auto keysDigest = util::DigestCalculator();
    native->applyDelta(reader, boundary);
    keysDigest << pageID << native->calculateDigest();
    for (const auto& m: migrants) {
        m.chunk->applyDelta(reader, boundary);
        keysDigest << m.id << m.chunk->calculateDigest();
    }

    if (keysDigest.CalculateDigest() != delta.finalDigest) {
        // ensure that the page will be rollback.
        throw std::invalid_argument("final digest of page[" + (std::string) pageID + "] is not valid");
    }

    // blockNumber == version is needed for creating new pages
    assert(blockNumber >= version);
    version = blockNumber;
}

Page::Migrant Page::extractMigrant(int32_fast index) {
    try {
        // todo needs optimization
        auto ret = Migrant(std::move(migrants.at(index).id), migrants.at(index).chunk.release());
        migrants.erase(migrants.begin() + index);
        return ret;
    } catch (const std::out_of_range&) {
        throw BlockError("invalid migrant index: " + std::to_string(index));
    }
}

Page::Chunk* Page::extractNative() {
    if (!migrants.empty()) throw BlockError("migrating native chunk of a page containing migrants");
    return native.release();
}

void Page::addMigrant(Migrant m) {
    // we don't need to check to see if a chunk with the same id exists.
    migrants.emplace_back(std::move(m));
}

void Page::setWritableFlag(bool writable) {
    native->setWritable(writable);
    for (const auto& m: migrants) {
        m.chunk->setWritable(writable);
    }
}

const std::vector<Page::Migrant>& Page::getMigrants() {
    return migrants;
}

Page::Chunk* Page::getNative() {
    return native.get();
}
