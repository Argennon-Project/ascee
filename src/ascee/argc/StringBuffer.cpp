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

#include "StringBuffer.h"
#include "util/PrefixTrie.hpp"

#define MAX_NUM64_LENGTH 32

using namespace argennon::ascee;
using namespace argennon::ascee::runtime;
using std::string_view, std::string;


inline static
std::size_t parse_scan(string_view str, int64_t& ret) {
    std::size_t pos;
    // string constructor copies its input, therefore we truncate the input str to make the copy less costly.
    ret = std::stoll(string(str.substr(0, MAX_NUM64_LENGTH)), &pos, 0);
    return pos;
}

inline static
std::size_t parse_scan(string_view str, double& ret) {
    std::size_t pos;
    ret = std::stod(string(str.substr(0, MAX_NUM64_LENGTH)), &pos);
    return pos;
}

/**
 *
 * @tparam T
 * @param pattern
 * @param output
 * @return
 */
template<typename T>
StringView StringView::scan(const StringView& pattern, T& output) const {
    string_view content = *this;
    int i = 0, j = 0;
    while (i < length() && j < pattern.length()) {
        if (!std::isspace(content[i])) {
            if (std::isspace(pattern[j])) ++j;
            else if (content[i] == pattern[j]) {
                ++i;
                ++j;
            } else break;
        } else if (std::isspace(pattern[j])) {
            if (content[i] == pattern[j]) i++;
            else ++j;
        } else break;
    }
    try {
        if (i == length() || j < pattern.length()) throw std::invalid_argument("pattern not found");

        auto pos = parse_scan(content.substr(i), output);
        return StringView(content.substr(i + pos));
    } catch (const std::invalid_argument&) {
        output = 0;
        return {};
    }
}

template StringView StringView::scan(const StringView&, int64_t&) const;

template StringView StringView::scan(const StringView&, double&) const;

bool StringView::isNull() { return data() == nullptr; }

StringView::StringView(const char* str) : string_view(str) {}

StringView::StringView(const char* str, size_type len) : basic_string_view(str, len) {}

StringView::StringView(const string_view& view) : basic_string_view(view) {}

int64_t StringView::parse(std::string_view str, const int64_t&) {
    // string constructor copies its input, therefore we truncate the input str to make the copy less costly.
    return std::stoll(std::string(str.substr(0, max_num64_length)), nullptr, 0);
}

double StringView::parse(std::string_view str, const double&) {
    return std::stod(std::string(str.substr(0, max_num64_length)), nullptr);
}

argennon::long_id StringView::parse(std::string_view str, const argennon::long_id&) {
    return util::PrefixTrie<uint64_t>::uncheckedParse(std::string(str));
}
