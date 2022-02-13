// Copyright (c) 2022 aybehrouz <behrouz_ayati@yahoo.com>. All rights
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

#ifndef ARGENNON_CORE_MOCK_MODIFIER_HPP
#define ARGENNON_CORE_MOCK_MODIFIER_HPP

#include <gmock/gmock.h>
#include "arg/primitives.h"

namespace argennon::mocking::ascee {

class MockModifier {
public:
    template<typename T>
    inline
    T load(int32 offset) { return {}; }

    template<typename T, int h>
    inline
    T loadVarUInt(const util::PrefixTrie <T, h>& trie, uint32 offset, int* n = nullptr) {
        return {};
    }

    template<typename T, int h>
    inline
    T loadIdentifier(const util::PrefixTrie <T, h>& trie, uint32 offset, uint32 index, int* n = nullptr) {
        return {};
    }

    template<typename T>
    inline
    void store(uint32 offset, T value, uint32 index = 0) {}

    template<typename T>
    inline
    void addInt(uint32 offset, T value) {}

    template<typename T, int h>
    inline
    int storeVarUInt(const util::PrefixTrie <T, h>& trie, uint32 offset, T value) {
        return 0;
    }

    bool isValid(int32 offset, int32 size) {
        return false;
    }

    uint32 getChunkSize() { return 0; };

    MOCK_METHOD(void, loadChunk, (long_id accountID, long_id localID));
    MOCK_METHOD(void, loadChunk, (long_id chunkID));
    MOCK_METHOD(void, loadContext, (long_id appID));
    MOCK_METHOD(void, writeToHeap, ());
    MOCK_METHOD(void, restoreVersion, (int16_t version));
    MOCK_METHOD(int16_t, saveVersion, ());
    MOCK_METHOD(void, updateChunkSize, (uint32 newSize));
};

} // namespace test::mocking::ascee
#endif // ARGENNON_CORE_MOCK_MODIFIER_HPP
