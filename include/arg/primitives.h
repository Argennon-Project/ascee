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
#include "util/PrefixTrie.hpp"

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
typedef struct FullID full_id;

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

class FullID {
public:
    constexpr FullID(__uint128_t id) : id(id) {} // NOLINT(google-explicit-constructor)

    FullID(long_id up, long_id down) { id = __uint128_t(up) << 64 | down; }

    operator __uint128_t() const { return id; } // NOLINT(google-explicit-constructor)

    explicit operator std::string() const {
        using namespace std;
        stringstream buf;
        buf << "0x" << hex << uint64_t(id >> 64) << ":" << hex << uint64_t(id);
        return {buf.str()};
    };

    /**
     * gives a 32-bit hash value.
     * @return
     */
    struct Hash {
        std::size_t operator()(full_id key) const noexcept {
            uint64_t prime1 = 8602280293;
            uint64_t prime2 = 17184414901;
            return uint64_t(key.id >> 64) % prime1 ^ uint64_t(key.id) % prime2;
        }
    };

private:
    __uint128_t id = 0;
};

constexpr long_id arg_app_id_g = 0x100000000000000;
constexpr long_id acc_id_mask_g = 0xffffffffffff0000;
constexpr uint16 nonce_start_g = 8;

} // namespace argennon
#endif // ASCEE_PRIMITIVES_H
