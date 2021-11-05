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
private:
    static_assert(height <= sizeof(T));
    T boundary[height] = {};

public:
    explicit IdentifierTrie(const std::array<T, height>& trie) {
        for (int i = 0; i < height; ++i) {
            boundary[i] = trie[i] << ((sizeof(T) - i - 1) << 3);
            if (i > 0 && boundary[i - 1] > boundary[i]) throw std::invalid_argument("malformed trie");
        }
    }

    int readIdentifier(T& id, const byte* binary, int maxLength = height) {
        id = 0;
        maxLength = maxLength < height ? maxLength : height;
        for (int i = 0; i < maxLength; ++i) {
            id |= T(binary[i]) << ((sizeof(T) - i - 1) << 3);
            printf("%lx <> %lx\n", id, boundary[i]);
            if (id < boundary[i]) return i + 1;
        }
        id = 0;
        throw std::out_of_range("invalid identifier");
    }

    int parseIdentifier(std::string symbolicRep, T& id) {
        // returned array by data is null-terminated after C++11
        byte buffer[height];
        char* token = strtok(symbolicRep.data(), ".");
        int i = 0;
        while (i < height && token != nullptr) {
            buffer[i++] = std::stoi(token, nullptr, 0);
            token = strtok(nullptr, ".");
        }
        return readIdentifier(id, buffer, i);
    }
};

} // namespace ascee
#endif // ASCEE_IDENTIFIER_TRIE_H
