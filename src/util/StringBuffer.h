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

#ifndef ASCEE_STRING_BUFFER_H
#define ASCEE_STRING_BUFFER_H

#define MAX_BUFFER_SIZE (2*1024*1024)

#include <string>
#include <cstring>
#include <stdexcept>
#include <array>

namespace ascee::runtime {

class StringView : public std::string_view {
public:
    StringView() = default;

    StringView(const char* cStr, size_type len);

    StringView(const char* cStr); // NOLINT(google-explicit-constructor)

    explicit StringView(const std::string_view& view);

    bool isNull();

    template<typename T>
    StringView scan(const StringView& pattern, T& output) const;

    //operator std::string_view() const;
};

/// This class dose not allocate any memory on heap and only uses stack. This is important for smart contract execution
/// when the amount of memory allocated on the heap is not measured.
template<int maxSize>
class StringBuffer {
    static_assert(maxSize < MAX_BUFFER_SIZE && maxSize >= 0);
public:
    auto append(const StringView& str) {
        if (maxSize < end + str.size()) throw std::out_of_range("append: str is too long");
        std::memcpy(buffer + end, str.data(), str.size());
        end += str.size();
        return this;
    }

    void clear() {
        end = 0;
    }

    explicit operator StringView() const {
        return StringView(buffer, end);
    }

private:
    int end = 0;
    // we allocate one extra byte, so always there will be a null at the end of buffer.
    char buffer[maxSize + 1] = {};
};

} // namespace ascee::runtime
#endif // ASCEE_STRING_BUFFER_H
