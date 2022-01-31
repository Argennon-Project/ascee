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

#ifndef ARGENNON_STATIC_ARRAY_H
#define ARGENNON_STATIC_ARRAY_H

#include <string>
#include <array>
#include <cassert>
#include <stdexcept>
#include <cstring>
#include "encoding.h"

namespace argennon::util {

template<typename T, std::size_t size>
class StaticArray : public std::array<T, size> {
public:
    StaticArray() = default;

    StaticArray(const std::array<T, size>& a) : std::array<T, size>(a) {} // NOLINT(google-explicit-constructor)

    explicit StaticArray(std::string_view base64) : std::array<T, size>() {
        auto sizeInBytes = size * sizeof(T);
        if (base64DecodeLen(base64.length()) > sizeInBytes) throw std::out_of_range("array is too small");
        auto modified = base64urlDecode(base64.data(), base64.length(), this->data());
        assert(modified <= sizeInBytes);
        std::memset(this->data() + modified, 0, sizeInBytes - modified);
    }

    std::string toBase64() {
        return base64urlEncode(this->data(), size);
    }

    StaticArray(const StaticArray&) { std::terminate(); }

    StaticArray& operator=(const StaticArray&) = delete;

    T* operator&() { // NOLINT(google-runtime-operator)
        return this->data();
    }

    std::string toString() {
        std::string result;
        for (const auto& elem: *this) {
            result += std::to_string(elem) + ",";
        }
        return result;
    }
};

} // namespace argennon::util
#endif // ARGENNON_STATIC_ARRAY_H
