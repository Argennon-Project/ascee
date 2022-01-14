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

namespace argennon::util {

template<typename T, std::size_t size>
class StaticArray : public std::array<T, size> {
public:
    StaticArray() = default;

    StaticArray(const StaticArray&) { std::terminate(); }

    StaticArray(StaticArray&&) = delete;

    StaticArray& operator=(const StaticArray&) = default;

    T* operator&() { // NOLINT(google-runtime-operator)
        return this->data();
    }

    std::string toString() {
        std::string result;
        for (const auto& elem: *this) {
            result += std::to_string(elem) + " ";
        }
        return result;
    }
};

} // namespace argennon::util
#endif // ARGENNON_STATIC_ARRAY_H
