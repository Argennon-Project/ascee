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

#ifndef ARGENNON_CORE_TRIES_H
#define ARGENNON_CORE_TRIES_H


#include <memory>
#include <cstring>
#include <sstream>
#include "util/PrefixTrie.hpp"
#include "primitives.h"

namespace argennon {

/// For decoding variable length unsigned integers
constexpr util::PrefixTrie<uint32_t, 4> var_uint_trie_g({0xd0, 0xf000, 0xfc0000, 0xffffff00});
/// For application identifiers
constexpr util::PrefixTrie<uint64_t, 3> app_trie_g({0xa0, 0xc000, 0xd00000});
/// For account identifiers
constexpr util::PrefixTrie<uint64_t, 3> account_trie_g({0x60, 0xd000, 0xe00000});
/// For generating local identifiers
constexpr util::PrefixTrie<uint64_t, 3> local_trie_g({0xc0, 0xe000, 0xf00000});

/**
 * A memory efficient representation of an identifier. It does not store the length of the identifier.
*/

class VarLenFullID {
    using byte = uint8_t;
public:
    /**
     * this constructor does not check its input.
     * @param binary MUST point to a valid identifier, otherwise the behaviour will be undefined.
     */
    explicit VarLenFullID(std::unique_ptr<byte[]>&& binary) : binary(std::move(binary)) {}

    VarLenFullID(const byte** binary, const byte* end) {
        auto start = *binary;
        app_trie_g.readPrefixCode(binary, end);
        account_trie_g.readPrefixCode(binary, end);
        local_trie_g.readPrefixCode(binary, end);
        auto len = int(*binary - start);
        this->binary = std::make_unique<byte[]>(len);
        memcpy(this->binary.get(), start, len);
    }

    VarLenFullID(const VarLenFullID& other) {
        auto len = other.getLen();
        binary = std::make_unique<byte[]>(len);
        memcpy(binary.get(), other.binary.get(), len);
    }

    VarLenFullID(VarLenFullID&&) = default;

    VarLenFullID& operator=(VarLenFullID&&) = default;

    VarLenFullID& operator=(const VarLenFullID&) = delete;

    bool operator==(const VarLenFullID& rhs) const {
        const byte* a = this->binary.get();
        const byte* b = rhs.binary.get();
        return app_trie_g.equals(a, b) &&
               account_trie_g.equals(a, b) &&
               local_trie_g.equals(a, b);
    }

    explicit operator FullID() const {
        const byte* ptr = binary.get();
        auto up = app_trie_g.readPrefixCode(&ptr);
        auto middle = account_trie_g.readPrefixCode(&ptr);
        auto down = local_trie_g.readPrefixCode(&ptr);
        return {up, {middle, down}};
    }

    [[nodiscard]]
    int getLen() const {
        const byte* ptr = binary.get();
        app_trie_g.readPrefixCode(&ptr);
        account_trie_g.readPrefixCode(&ptr);
        local_trie_g.readPrefixCode(&ptr);
        return int(ptr - binary.get());
    }


    [[nodiscard]]
    const byte* getBinary() const {
        return binary.get();
    }

    /**
     * gives a 32-bit hash value.
     * @return
     */
    struct Hash {
        std::size_t operator()(const VarLenFullID& key) const noexcept {
            return FullID::Hash{}(FullID(key));
        }
    };

    explicit operator std::string() const {
        std::stringstream buf;
        buf << "0x";
        for (int i = 0; i < getLen(); ++i) {
            buf << std::hex << (int) binary[i];
        }
        return buf.str();
    }

private:
    // because this is a memory efficient representation we don't use a vector, since a vector would store the 64-bit
    // size of the identifier.
    std::unique_ptr<byte[]> binary;
};

} // argennon
#endif // ARGENNON_CORE_TRIES_H
