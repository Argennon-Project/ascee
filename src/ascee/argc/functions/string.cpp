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

#include <string>

#include <argc/types.h>
#include <argc/functions.h>

using std::string;
using namespace argennon;
using namespace ascee;

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

signature_c argc::sig_match_pattern(string_view_c str, string_view_c start, string_view_c end, int32& pos) {
    return str.matchPattern<signature_c>(start, end, pos);
}

int64 argc::int64_match_pattern(string_view_c str, string_view_c start, string_view_c end, int32& pos) {
    return str.matchPattern<int64>(start, end, pos);
}

long_id argc::long_id_match(string_view_c str, string_view_c start, string_view_c end, int32& pos) {
    return str.matchPattern<long_id>(start, end, pos);
}

string_view_c argc::str_match(string_view_c str, string_view_c start, string_view_c end, int32& pos) {
    return str.matchPattern<string_view_c>(start, end, pos);
}

int64 argc::scan_int64(string_view_c input, string_view_c pattern, string_view_c& rest) {
    int64 ret;
    rest = input.scan(pattern, ret);
    return ret;
}

float64 argc::scan_float64(string_view_c input, string_view_c pattern, string_view_c& rest) {
    float64 ret;
    rest = input.scan(pattern, ret);
    return ret;
}

