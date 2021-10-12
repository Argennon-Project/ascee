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

using std::string;
using namespace ascee;

extern "C"
void argcrt::append_str(string_buffer* buf, string_t str) {
    if (buf->maxSize < buf->end + str.length) {
        raise(SIGSEGV);
    }
    strncpy(buf->buffer + buf->end, str.content, str.length);
    if (str.content[str.length - 1] == '\0')
        buf->end += str.length - 1;
    else
        buf->end += str.length;
}

extern "C"
void argcrt::append_int64(string_buffer* buf, int64 i) {
    string str = std::to_string(i);
    append_str(buf, string_t{str.c_str(), static_cast<int>(str.size() + 1)});
}

extern "C"
void argcrt::clear_buffer(string_buffer* buf) {
    buf->end = 0;
}

extern "C"
string_t buf_to_string(const string_buffer* buf) {
    return string_t{
            .content = buf->buffer,
            .length = buf->end
    };
}
