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

#include <csignal>
#include <cstring>
#include <string>
#include <stdexcept>

#include <argc/types.h>
#include <argc/functions.h>

#define MAX_NUM64_LENGTH 32

using std::string, std::string_view;
using namespace ascee;

extern "C"
void argcrt::append_str(string_buffer* buf, string_t str) {
    // we require the size to be one byte more than the actual required amount
    if (buf->maxSize <= buf->end + str.length) raise(SIGSEGV);
    memcpy(buf->buffer + buf->end, str.content, str.length);
    buf->end += str.length;
    buf->buffer[buf->end] = '\0';
}

extern "C"
void argcrt::append_int64(string_buffer* buf, int64 i) {
    string str = std::to_string(i);
    append_str(buf, string_t{str.c_str(), static_cast<int32>(str.size())});
}

extern "C"
void argcrt::clear_buffer(string_buffer* buf) {
    buf->end = 0;
}

inline static
int32 parse(string_view str, int64* ret) {
    size_t pos;
    // string constructor copies its input, therefore we truncate the input str to make the copy less costly.
    *ret = std::stol(string(str.substr(0, MAX_NUM64_LENGTH)), &pos);
    return static_cast<int32>(pos);
}

inline static
int32 parse(string_view str, float64* ret) {
    size_t pos;
    *ret = std::stod(string(str.substr(0, MAX_NUM64_LENGTH)), &pos);
    return static_cast<int32>(pos);
}

template<typename T>
static
T scan(string_t input, string_t pattern, string_t* rest) {
    int i = 0, j = 0;
    while (i < input.length && j < pattern.length) {
        if (!std::isspace(input.content[i])) {
            if (std::isspace(pattern.content[j])) j++;
            else if (input.content[i] == pattern.content[j]) {
                i++;
                j++;
            } else break;
        } else if (std::isspace(pattern.content[j])) {
            if (input.content[i] == pattern.content[j]) i++;
            else j++;
        } else break;
    }
    T ret{};
    try {
        if (i == input.length || j < pattern.length) throw std::invalid_argument("pattern not found");

        // string_view constructor DOES NOT make a copy of its input.
        string_view targetStr(input.content + i, input.length - i);
        int32 pos = parse(targetStr, &ret);
        rest->content = input.content + i + pos;
        rest->length = static_cast<int32>(input.length - i - pos);
    } catch (const std::invalid_argument&) {
        rest->content = nullptr;
        rest->length = -1;
    }
    return ret;
}

extern "C"
int64 argcrt::scan_int64(string_t input, string_t pattern, string_t* rest) {
    return scan<int64>(input, pattern, rest);
}

extern "C"
float64 argcrt::scan_float64(string_t input, string_t pattern, string_t* rest) {
    return scan<float64>(input, pattern, rest);
}

extern "C"
string_t buf_to_string(const string_buffer* buf) {
    return string_t{
            .content = buf->buffer,
            .length = buf->end
    };
}
