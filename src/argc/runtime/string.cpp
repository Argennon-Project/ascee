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
#include <argc/functions.h>

#include "argc/types.h"

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

extern "C"
int64 argcrt::scan_int64(string_t input, string_t pattern, string_t* rest = nullptr) {
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
    if (j < pattern.length) {
        if (rest != nullptr) {
            rest->content = nullptr;
            rest->length = -1;
        }
        return 0;
    }
    string numStr(input.content + i, std::min(64, input.length - i));
    size_t pos;
    int64 ret = std::stoi(numStr, &pos);
    if (rest != nullptr) {
        rest->content = input.content + i + pos;
        rest->length = static_cast<int32>(input.length - i - pos);
    }
    return ret;
}

extern "C"
string_t buf_to_string(const string_buffer* buf) {
    return string_t{
            .content = buf->buffer,
            .length = buf->end
    };
}
