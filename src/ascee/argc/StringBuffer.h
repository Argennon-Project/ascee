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

#ifndef ARGENNON_STRING_BUFFER_H
#define ARGENNON_STRING_BUFFER_H

#define MAX_BUFFER_SIZE (2*1024*1024)

#include "core/primitives.h"
#include <string>
#include <stdexcept>
#include <climits>
#include "util/StaticArray.hpp"

namespace argennon::ascee::runtime {

class StringView {
public:
    static constexpr int max_num64_length = 32;

    StringView() = default;

    StringView(const char* cStr, size_t len) : view(cStr, len) {}

    StringView(const char* cStr) : view(cStr) {} // NOLINT(google-explicit-constructor)

    StringView(std::string_view view) : view(view) {} // NOLINT(google-explicit-constructor)

    StringView(const std::string& str) : view(str) {} // NOLINT(google-explicit-constructor)

    void operator=(const StringView&) = delete;

    bool operator==(const StringView& other) const {
        return view == other.view;
    }

    // should be explicit since string_view is not safe
    explicit operator std::string_view() const {
        return view;
    }

    explicit operator std::string() const;

    [[nodiscard]]
    auto data() const {
        return view.data();
    }

    [[nodiscard]]
    int size() const {
        if (view.size() > INT_MAX) throw std::runtime_error("string is too big");
        return (int) view.size();
    }

    [[nodiscard]]
    auto length() const {
        return view.length();
    }

    template<typename T>
    StringView scan(const StringView& pattern, T& output) const;

    /**
     *
     * @tparam T
     * @param start
     * @param end
     * @param[in,out] pos
     * @return
     */
    template<typename T>
    T matchPattern(StringView start, StringView end, int32& pos) {
        auto foundPos = view.find((std::string_view) start, pos);
        if (foundPos == std::string_view::npos) {
            throw std::invalid_argument("start pattern:" + std::string(start) + " not found");
        }

        auto startPos = foundPos + start.size();

        foundPos = view.find((std::string_view) end, startPos);
        if (foundPos == std::string_view::npos) {
            throw std::invalid_argument("end pattern:" + std::string(end) + " not found");
        }

        auto len = foundPos - startPos;
        pos = int32(foundPos + end.size());
        return parse(view.substr(startPos, len), T{});
    }

private:
    const std::string_view view;

    static int64_t parse(std::string_view str, const int64_t&);

    static double parse(std::string_view str, const double&);

    static long_id parse(std::string_view str, const long_id&);

    template<typename T>
    static T parse(std::string_view str, const T&) {
        return T(str);
    }
};

/// This class dose not allocate any memory on heap and only uses stack. This is important for smart contract execution
/// when the amount of memory allocated on the heap is not measured.
template<int maxSize>
class StringBuffer {
    static_assert(maxSize < MAX_BUFFER_SIZE && maxSize >= 0);
public:
    StringBuffer& append(StringView str) {
        if (maxSize < end + str.size()) throw std::out_of_range("append: str is too long");
        util::memCopy(buffer + end, str.data(), str.size());
        end += str.size();
        return *this;
    }

    StringBuffer& operator<<(StringView str) {
        return append(str);
    }

    StringBuffer& operator<<(const LongID& v) {
        return append(std::string(v));
    }

    StringBuffer& operator<<(uint64_t v) {
        return append(std::to_string(v));
    }

    [[nodiscard]] int size() const { return end; }

    StringBuffer& clear() {
        end = 0;
        return *this;
    }

    // since StringView is safe this can be non-explicit
    operator StringView() const { // NOLINT(google-explicit-constructor)
        return StringView(buffer, end);
    }

    // should be explicit since string_view is unsafe
    explicit operator std::string_view() const {
        return std::string_view(buffer, end);
    }

    explicit operator std::string() const {
        return std::string(buffer, end);
    }

    void operator=(const StringBuffer&) = delete;

private:
    int end = 0;
    // we allocate one extra byte, so always there will be a null at the end of the buffer.
    char buffer[maxSize + 1] = {};
};

} // namespace argennon::ascee::runtime
#endif // ARGENNON_STRING_BUFFER_H
