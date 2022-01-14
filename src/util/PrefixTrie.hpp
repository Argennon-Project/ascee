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

#ifndef ARGENNON_IDENTIFIER_TRIE_H
#define ARGENNON_IDENTIFIER_TRIE_H

#include <argc/types.h>
#include <string>
#include <stdexcept>
#include <array>

namespace argennon::util {

template<typename T, int height = sizeof(T)>
class PrefixTrie {
    static_assert(height <= sizeof(T) && height > 0);
    static_assert(std::is_unsigned<T>::value);
public:
    explicit PrefixTrie(const std::array<T, height>& trie) {
        for (int i = 0; i < height; ++i) {
            auto shift = (sizeof(T) - i - 1) << 3;
            this->trie[i] = trie[i] & (~T(0) >> shift);
            boundary[i] = trie[i] << shift;
            if (i > 0 && boundary[i - 1] > boundary[i]) throw std::invalid_argument("malformed trie");
        }

        // here we should use this->trie to make sure that high order bits are properly zeroed.
        sum[0] = this->trie[0];
        for (int i = 1; i < height; ++i) {
            sum[i] = sum[i - 1] + this->trie[i] - (this->trie[i - 1] << 8);
        }
    }

    T readPrefixCode(const byte* binary, int* len = nullptr, int maxLength = height) const {
        T id = 0;
        if (maxLength > height) maxLength = height;
        for (int i = 0; i < maxLength; ++i) {
            id |= T(binary[i]) << ((sizeof(T) - i - 1) << 3);
            if (id < boundary[i]) {
                if (len != nullptr) *len = i + 1;
                return id;
            }
        }
        if (len != nullptr) *len = 0;
        throw std::out_of_range("readPrefixCode: invalid identifier");
    }

    T readPrefixCode(T binary, int* len = nullptr, int maxLength = height) const {
        if (maxLength > height) maxLength = height;
        for (int i = 0; i < maxLength; ++i) {
            if (binary < boundary[i]) {
                if (len != nullptr) *len = i + 1;
                return binary & (~T(0) << ((sizeof(T) - i - 1) << 3));
            }
        }
        if (len != nullptr) *len = 0;
        throw std::out_of_range("readPrefixCode: invalid identifier");
    }

    void parsePrefixCode(std::string symbolicRep, T& id) const {
        byte buffer[height];
        // returned array by data() is null-terminated after C++11
        char* token = strtok(symbolicRep.data(), ".");
        int i = 0;
        while (token != nullptr) {
            if (i == height) throw std::invalid_argument("parsePrefixCode: input too long");

            auto component = std::stoul(token, nullptr, 0);
            if (component > 255) throw std::overflow_error("identifier component is larger than 255");

            buffer[i++] = component;
            token = strtok(nullptr, ".");
        }
        int n;
        id = readPrefixCode(buffer, &n, i);
        if (n != i) throw std::invalid_argument("parsePrefixCode: input too long");
    }

    T encodeVarUInt(T value, int* len = nullptr) const {
        for (int i = 0; i < height; ++i) {
            if (value < sum[i]) {
                auto bound = trie[i];
                if (len != nullptr) *len = i + 1;
                return (bound - (sum[i] - value)) << ((sizeof(T) - i - 1) << 3);
            }
        }
        if (len != nullptr) *len = 0;
        throw std::overflow_error("encodeVarUInt: value too large");
    }

    template<typename U>
    T decodeVarUInt(U binary, int* len = nullptr, int maxLength = height) const {
        int n;
        T code = readPrefixCode(binary, &n, maxLength);
        code >>= (sizeof(T) - n) << 3;

        auto bound = trie[n - 1];
        if (len != nullptr) *len = n;
        return sum[n - 1] + code - bound;
    }

    void writeBigEndian(byte* dest, T value, int n) const {
        for (int i = 0; i < n; ++i) {
            dest[i] = byte(value >> ((sizeof(T) - i - 1) << 3));
        }
    }

private:
    T boundary[height] = {};
    T sum[height] = {};
    T trie[height] = {};
};

inline const PrefixTrie<uint16_t, 2> gNonceTrie({0xe0, 0xff00});

inline const PrefixTrie<uint64_t, 6> gAppTrie({});

} // namespace argennon::util
#endif // ARGENNON_IDENTIFIER_TRIE_H
