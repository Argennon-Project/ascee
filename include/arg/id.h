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

#ifndef ARGENNON_CORE_ID_H
#define ARGENNON_CORE_ID_H


#include <cstdint>
#include <sstream>
#include "primitives.h"
#include <memory>
#include "util/PrefixTrie.hpp"
#include "util/crypto/DigestCalculator.h"

namespace argennon {


constexpr util::PrefixTrie<uint32_t, 4>
        var_uint_trie_g({0xd0, 0xf000, 0xfc0000, 0xffffff00});  // for decoding variable length unsigned integers
constexpr util::PrefixTrie<uint64_t, 3> app_trie_g({0xa0, 0xc000, 0xd00000});       // for application identifiers
constexpr util::PrefixTrie<uint64_t, 3> account_trie_g({0x60, 0xd000, 0xe00000});   // for account identifiers
constexpr util::PrefixTrie<uint64_t, 3> local_trie_g({0xc0, 0xe000, 0xf00000});     // for generating local identifiers

class LongID {
public:
    LongID() = default;

    explicit LongID(std::string_view str) : id(util::PrefixTrie<uint64_t>::uncheckedParse(std::string(str))) {}

    constexpr LongID(uint64_t id) noexcept: id(id) {} // NOLINT(google-explicit-constructor)

    operator uint64_t() const { return id; } // NOLINT(google-explicit-constructor)

    explicit operator std::string() const {
        std::stringstream buf;
        buf << "0x" << std::hex << id;
        return {buf.str()};
    };

private:
    uint64_t id = 0;
};

class LongLongID {
public:
    constexpr LongLongID(long_id up, long_id down) : up(up), down(down) {}

    explicit operator std::string() const {
        using namespace std;
        return (string) up + "::" + (string) down;
    };

    // this is a small class, we pass it by value.
    bool operator<(LongLongID rhs) const {
        return up < rhs.up || (up == rhs.up && down < rhs.down);
    }

    bool operator==(LongLongID rhs) const {
        return up == rhs.up && down == rhs.down;
    }

    /**
    * gives a 32-bit hash value.
    * @return
    */
    struct Hash {
        std::size_t operator()(LongLongID key) const noexcept {
            uint64_t prime1 = 8602280293;
            uint64_t prime2 = 17184414901;
            return uint64_t(key.up) % prime1 ^ uint64_t(key.down) % prime2;
        }
    };

private:
    const long_id up;
    const long_id down;
};

class FullID {
public:
    constexpr FullID(long_id up, long_long_id down) : up(up), down(down) {}

    explicit operator std::string() const {
        using namespace std;
        return (string) up + "::" + (string) down;
    };

    // this is a small class, we pass it by value.
    bool operator<(const FullID& rhs) const {
        return up < rhs.up || (up == rhs.up && down < rhs.down);
    }

    bool operator==(const FullID& rhs) const {
        return up == rhs.up && down == rhs.down;
    }

    /**
     * gives a 32-bit hash value.
     * @return
     */
    struct Hash {
        std::size_t operator()(const FullID& key) const noexcept {
            uint64_t prime = 15236479477;
            return uint64_t(key.up) % prime ^ long_long_id::Hash{}(key.down);
        }
    };

private:
    const long_id up;
    const long_long_id down;
};

/**
 * A memory efficient representation of an identifier. It does not store the length of the identifier.
 */
class VarLenFullID {
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

    friend util::DigestCalculator& operator<<(util::DigestCalculator& lhs, const VarLenFullID& id) {
        return lhs.append(id.binary.get(), id.getLen());
    }

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

constexpr long_id arg_app_id_g = 0x100000000000000;
constexpr uint16_t nonce_start_g = 8;

} // namespace argennon
#endif // ARGENNON_CORE_ID_H
