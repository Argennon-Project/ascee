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

#ifndef ASCEE_PRIMITIVES_H
#define ASCEE_PRIMITIVES_H

#include <cstdint>
#include <sstream>
#include <memory>
#include "util/PrefixTrie.hpp"
#include "util/crypto/DigestCalculator.h"

namespace argennon {

/// int represents a signed integer with the most efficient size for the platform which MUST NOT be smaller than 32 bits.
typedef uint8_t byte;
typedef uint16_t uint16;
typedef int32_t int32;
typedef int_fast32_t int32_fast;
typedef int64_t int64;
typedef int_fast64_t int64_fast;
typedef __int128_t int128;
typedef double float64;
//typedef __float128 float128;
typedef uint32_t short_id;
typedef struct LongID long_id;
typedef struct LongLongID long_long_id;
typedef struct FullID full_id;

inline const util::PrefixTrie<uint16_t, 2> gNonceTrie({0xe0, 0xff00});
inline const util::PrefixTrie<uint32_t, 4> gVarSizeTrie({0xd0, 0xf000, 0xfc0000, 0xffffff00});
inline const util::PrefixTrie<uint64_t, 2> app_trie_g({0x90, 0xb000});
inline const util::PrefixTrie<uint64_t, 2> account_trie_g({0x80, 0xc000});
inline const util::PrefixTrie<uint64_t, 2> local_trie_g({0xb0, 0xc800});

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
class VarLenID {
public:
    explicit VarLenID(std::unique_ptr<byte[]>&& binary) : binary(std::move(binary)) {}

/*
    VarLenID(std::unique_ptr<byte[]>&& binary, int maxLen, int* actualLen) : binary(std::move(binary)) {
        const byte* ptr = binary.get();
        app_trie_g.readPrefixCode(&ptr, maxLen);
        account_trie_g.readPrefixCode(&ptr, maxLen);
        local_trie_g.readPrefixCode(&ptr, maxLen);
    }
*/
    VarLenID(const VarLenID& other) {
        auto len = other.getLen();
        binary = std::make_unique<byte[]>(len);
        memcpy(binary.get(), other.binary.get(), len);
    }

    VarLenID(VarLenID&&) = default;

    VarLenID& operator=(VarLenID&&) = default;

    VarLenID& operator=(const VarLenID&) = delete;

    friend util::DigestCalculator& operator<<(util::DigestCalculator& lhs, VarLenID id) {
        return lhs.append(id.binary.get(), id.getLen());
    }

    bool operator==(const VarLenID& rhs) const {
        auto len = getLen();
        for (int i = 0; i < len; ++i) {
            if (binary[i] != rhs.binary[i]) return false;
        }
        return true;
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
        std::size_t operator()(const VarLenID& key) const noexcept {
            return FullID::Hash{}(FullID(key));
        }
    };

    explicit operator std::string() const {
        std::stringstream buf("0x");
        for (int i = 0; i < getLen(); ++i) {
            buf << std::hex << binary[i];
        }
        return buf.str();
    }

private:
    // because this is a memory efficient representation we don't use a vector, since a vector would store the 64-bit
    // size of the identifier.
    std::unique_ptr<byte[]> binary;
};

constexpr long_id arg_app_id_g = 0x100000000000000;
constexpr uint16 nonce_start_g = 8;

} // namespace argennon
#endif // ASCEE_PRIMITIVES_H
