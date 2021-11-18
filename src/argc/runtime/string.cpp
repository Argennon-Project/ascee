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

#include <string>

#include <argc/types.h>
#include <argc/functions.h>

using std::string, std::string_view;
using namespace ascee;
using namespace ascee::runtime;

/*
template<int maxSize>
void argcrt::append_str(string_buffer2<maxSize>& buf, const string_t& str) {
    buf.append(str);
}

template<int maxSize>
void argcrt::append_int64(string_buffer2<maxSize>& buf, int64 i) {
    buf.append(string_t(std::to_string(i)));
}

template<int maxSize>
void argcrt::append_float64(string_buffer2<maxSize>& buf, float64 f) {
    buf.append(string_t(std::to_string(f)));
}

template<int maxSize>
void argcrt::clear_buffer(string_buffer2<maxSize>& buf) {
    buf.clear();
}

void buf_to_string(const string_buffer& buf, string_t &str) {
    str = string_t(buf);
}
*/

int64 argc::scan_int64(const string_t& input, const string_t& pattern, string_t& rest) {
    int64 ret;
    rest = input.scan(pattern, ret);
    return ret;
}

float64 argc::scan_float64(const string_t& input, const string_t& pattern, string_t& rest) {
    float64 ret;
    rest = input.scan(pattern, ret);
    return ret;
}

