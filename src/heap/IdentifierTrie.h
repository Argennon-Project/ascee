// Copyright (c) 2021 aybehrouz <behrouz_ayati@yahoo.com>. All rights reserved.
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

namespace ascee {

template<typename T, int height = sizeof(T)>
class IdentifierTrie {
    static_assert(height <= sizeof(T));
    static_assert(std::is_unsigned<T>::value);
private:
    T boundary[height] = {};

public:
    explicit IdentifierTrie(const std::array<T, height>& trie) {
        for (int i = 0; i < height; ++i) {
            boundary[i] = trie[i] << ((sizeof(T) - i - 1) << 3);
            if (i > 0 && boundary[i - 1] > boundary[i]) throw std::invalid_argument("malformed trie");
        }
    }

    int readIdentifier(const byte* binary, T& id, int maxLength = height) {
        id = 0;
        if (maxLength > height) maxLength = height;
        for (int i = 0; i < maxLength; ++i) {
            id |= T(binary[i]) << ((sizeof(T) - i - 1) << 3);
            if (id < boundary[i]) return i + 1;
        }
        id = 0;
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
        int n = readIdentifier(buffer, id, i);
        if (n != i) throw std::invalid_argument("parseIdentifier: input too long");
    }
};

} // namespace ascee
#endif // ASCEE_IDENTIFIER_TRIE_H
