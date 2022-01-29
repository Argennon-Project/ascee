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

void Page::applyDelta(full_id pageID, const Page::Delta& delta, int64_fast blockNumber) {
    auto keysDigest = util::DigestCalculator();
    auto boundary = delta.content.data() + delta.content.size();

    auto reader = native->applyDelta(delta.content.data(), boundary);
    keysDigest << pageID << native->calculateDigest();
    for (const auto& item: migrants) {
        reader = item.second->applyDelta(reader, boundary);
        keysDigest << item.first << item.second->calculateDigest();
    }

    if (keysDigest.CalculateDigest() != delta.finalDigest) {
        // ensure that the page will be rollback.
        throw std::invalid_argument("final digest of page[" + (std::string) pageID + "] is not valid");
    }

    // = is needed for creating new pages
    assert(blockNumber >= version);
    version = blockNumber;
}

Page::Chunk* Page::extractMigrant(full_id id) {
    try {
        auto ret = migrants.at(id).release();
        migrants.erase(id);
        return ret;
    } catch (const std::out_of_range&) {
        throw BlockError("is not a migrant");
    }
}

Page::Chunk* Page::extractNative() {
    if (!migrants.empty()) throw BlockError("migrating native chunk of a page containing migrants");
    return native.release();
}

void Page::addMigrant(full_id id, Chunk* migrant) {
    if (!migrants.try_emplace(id, migrant).second) {
        throw BlockError("migrant already exists");
    }
}

void Page::setWritableFlag(bool writable) {
    native->setWritable(writable);
    for (const auto& pair: migrants) {
        pair.second->setWritable(writable);
    }
}

const std::map<full_id, std::unique_ptr<Page::Chunk>>& Page::getMigrants() {
    return migrants;
}

Page::Chunk* Page::getNative() {
    return native.get();
}
