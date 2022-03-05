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

#ifndef ARGENNON_CORE_HEAP_ID_H
#define ARGENNON_CORE_HEAP_ID_H

#include <memory>

namespace argennon::ascee::runtime {


class LongID {
public:
    LongID() = default;

    explicit LongID(std::string_view str);

    constexpr LongID(uint64_t id) noexcept: id(id) {} // NOLINT(google-explicit-constructor)

    operator uint64_t() const { return id; } // NOLINT(google-explicit-constructor)

    explicit operator std::string() const;

private:
    uint64_t id = 0;
};

class LongLongID {
public:
    constexpr LongLongID(LongID up, LongID down) : up(up), down(down) {}

    explicit operator std::string() const;

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

    explicit operator std::string() const;;

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

    VarLenFullID(const byte** binary, const byte* end);

    VarLenFullID(const VarLenFullID& other);

    VarLenFullID(VarLenFullID&&) = default;

    VarLenFullID& operator=(VarLenFullID&&) = default;

    VarLenFullID& operator=(const VarLenFullID&) = delete;

    bool operator==(const VarLenFullID& rhs) const;

    explicit operator FullID() const;

    [[nodiscard]]
    int getLen() const;

    [[nodiscard]]
    const byte* getBinary() const;

    /**
     * gives a 32-bit hash value.
     * @return
     */
    struct Hash {
        std::size_t operator()(const VarLenFullID& key) const noexcept {
            return FullID::Hash{}(FullID(key));
        }
    };

    explicit operator std::string() const;

private:
    // because this is a memory efficient representation we don't use a vector, since a vector would store the 64-bit
    // size of the identifier.
    std::unique_ptr<byte[]> binary;
};


} // namespace argennon::ascee::runtime
#endif // ARGENNON_CORE_HEAP_ID_H
