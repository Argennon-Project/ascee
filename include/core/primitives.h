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

#include <cinttypes>
#include <string>
#include "util/encoding.h"

namespace argennon {

typedef uint8_t byte;
typedef uint16_t uint16;
typedef int16_t int16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int_fast32_t int32_fast;
typedef int64_t int64;
typedef int_fast64_t int64_fast;
typedef __int128_t int128;
typedef double float64;
//typedef __float128 float128;

typedef uint32_t short_id;

class LongID {
public:
    LongID() = default;

    constexpr LongID(uint64_t id) noexcept: id(id) {} // NOLINT(google-explicit-constructor)

    operator uint64_t() const { return id; } // NOLINT(google-explicit-constructor)

    explicit operator std::string() const {
        return util::toHex(id);
    }

private:
    uint64_t id = 0;
};

class LongLongID {
public:
    constexpr LongLongID(LongID up, LongID down) : up(up), down(down) {}

    explicit operator std::string() const {
        return (std::string) up + "::" + (std::string) down;
    }


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
    const LongID up;
    const LongID down;
};

class FullID {
public:
    constexpr FullID(LongID up, LongLongID down) : up(up), down(down) {}

    explicit operator std::string() const {
        return (std::string) up + "::" + (std::string) down;
    }

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
            return uint64_t(key.up) % prime ^ LongLongID::Hash{}(key.down);
        }
    };

private:
    const LongID up;
    const LongLongID down;
};

using long_id = LongID;
using long_long_id = LongLongID;
using full_id = FullID;

constexpr long_id arg_app_id_g = 0x100000000000000;
constexpr uint16_t nonce_start_g = 8;

} // namespace argennon
#endif // ASCEE_PRIMITIVES_H
