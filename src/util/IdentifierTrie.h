// Copyright (c) 2021-2021 aybehrouz <behrouz_ayati@yahoo.com>. All rights reserved.
// This file is part of the C++ implementation of the Argennon smart contract
// Execution Environment (AscEE).
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

#ifndef ASCEE_IDENTIFIER_TRIE_H
#define ASCEE_IDENTIFIER_TRIE_H

#include <argc/types.h>
#include <string>
#include <stdexcept>
#include <array>
#include <cstring>

namespace ascee::runtime {

template<typename T, int height = sizeof(T)>
class IdentifierTrie {
    static_assert(height <= sizeof(T) && height > 0);
    static_assert(std::is_unsigned<T>::value);
public:
    explicit IdentifierTrie(const std::array<T, height>& trie) {
        for (int i = 0; i < height; ++i) {
            boundary[i] = trie[i] << ((sizeof(T) - i - 1) << 3);
            if (i > 0 && boundary[i - 1] > boundary[i]) throw std::invalid_argument("malformed trie");
        }

        count[0] = trie[0] & 0x0ff;
        for (int i = 1; i < height; ++i) {
            count[i] = (boundary[i] - boundary[i - 1]) >> ((sizeof(T) - i - 1) << 3);
        }
    }

    T readIdentifier(const byte* binary, int* len = nullptr, int maxLength = height) {
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
        throw std::out_of_range("readIdentifier: invalid identifier");
    }

    void parseIdentifier(std::string symbolicRep, T& id) {
        byte buffer[height];
        // returned array by data() is null-terminated after C++11
        char* token = strtok(symbolicRep.data(), ".");
        int i = 0;
        while (token != nullptr) {
            if (i == height) throw std::invalid_argument("parseIdentifier: input too long");

            auto component = std::stoul(token, nullptr, 0);
            if (component > 255) throw std::overflow_error("identifier component is larger than 255");

            buffer[i++] = component;
            token = strtok(nullptr, ".");
        }
        int n;
        id = readIdentifier(buffer, &n, i);
        if (n != i) throw std::invalid_argument("parseIdentifier: input too long");
    }

    int encodeVarUInt(T value, byte* buffer) {
        auto sum = 0;
        for (int i = 0; i < height; ++i) {
            sum += count[i];
            if (value < sum) {
                auto bound = boundary[i] >> ((sizeof(T) - i - 1) << 3);
                write(bound - (sum - value), buffer, i + 1);
                return i + 1;
            }

        }
        throw std::overflow_error("encodeVarUInt: value too large");
    }

    T decodeVarUInt(const byte* binary, int* len = nullptr, int maxLength = height) {
        int n;
        T code = readIdentifier(binary, &n, maxLength);
        code >>= (sizeof(T) - n) << 3;

        T sum = 0;
        for (int i = 0; i < n; ++i) sum += count[i];

        auto bound = boundary[n - 1] >> ((sizeof(T) - n) << 3);
        if (len != nullptr) *len = n;
        return sum + code - bound;
    }

private:
    T boundary[height] = {};
    T count[height] = {};

    void write(T value, byte* buffer, int n) {
        for (int i = 0; i < n; ++i) {
            buffer[i] = byte(value >> ((n - i - 1) << 3));
        }
    }
};

} // namespace ascee::runtime
#endif // ASCEE_IDENTIFIER_TRIE_H
