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
#include <stdexcept>
#include "encoding.h"

namespace argennon::util {

template<typename T, std::size_t sizeValue>
class StaticArray {
public:
    StaticArray() = default;

    StaticArray(const std::array<T, sizeValue>& a) : content(a) {} // NOLINT(google-explicit-constructor)

    explicit StaticArray(uint8_t* binary) {
        auto sizeInBytes = sizeValue * sizeof(T);
        util::memCopy(content.data(), binary, sizeInBytes);
    }

    explicit StaticArray(std::string_view base64) {
        auto sizeInBytes = sizeValue * sizeof(T);
        if (base64DecodeLen(base64.length()) > sizeInBytes) throw std::out_of_range("array is too small");
        auto modified = base64urlDecode(base64.data(), base64.length(), content.data());
        util::memSet(content.data() + modified, 0, sizeInBytes - modified);
    }

    StaticArray& operator=(const StaticArray&) = delete;

    T* operator&() { // NOLINT(google-runtime-operator)
        return content.data();
    }

    T& operator[](std::size_t idx) { return content.at(idx); }

    const T& operator[](std::size_t idx) const { return content.at(idx); }

    auto data() {
        return content.data();
    }

    auto size() const {
        return sizeValue;
    }

    [[nodiscard]]
    std::string toBase64() const {
        return base64urlEncode(content.data(), sizeValue);
    }

    [[nodiscard]]
    std::string toString() const {
        std::string result;
        for (const auto& elem: content) {
            result += std::to_string(elem) + ",";
        }
        return result;
    }

private:
    std::array<T, sizeValue> content;
};

} // namespace argennon::util
#endif // ARGENNON_STATIC_ARRAY_H
