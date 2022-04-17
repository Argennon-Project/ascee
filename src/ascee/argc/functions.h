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

#ifndef ARGENNON_ARGC_FUNCTIONS_H
#define ARGENNON_ARGC_FUNCTIONS_H

#include <argc/types.h>

namespace argennon::ascee::argc {

int64 load_int64(int32 offset);

int invoke_dispatcher(byte forwarded_gas, long_id app_id, response_buffer_c& response, string_view_c request);

void enter_area();

void exit_area();

float64 safe_addf64(float64 a, float64 b);

float64 truncate_float64(float64 f, int n);

template<int maxSize>
void append_str(string_buffer_c <maxSize>& buf, string_view_c str) {
    buf.append(str);
}

template<int maxSize>
void append_long_id(string_buffer_c <maxSize>& buf, long_id id) {
    buf.append(std::string(id));
}

template<int maxSize>
void append_int64(string_buffer_c <maxSize>& buf, int64 i) {
    buf.append(runtime::StringView((std::to_string(i))));
}

template<int maxSize>
void append_float64(string_buffer_c <maxSize>& buf, float64 f) {
    buf.append(std::to_string(f));
}

template<int maxSize>
void clear_buffer(string_buffer_c <maxSize>& buf) {
    buf.clear();
}

template<int maxSize>
string_view_c buf_to_string(const string_buffer_c <maxSize>& buf) {
    return string_view_c(buf);
}

float64 exact_addf64(float64 a, float64 b);

int dependant_call(long_id app_id, response_buffer_c& response, string_view_c request);

void invoke_deferred(long_id app_id, response_buffer_c& response, string_view_c request);

void revert(string_view_c msg);

signature_c p_scan_sig(string_view_c str, string_view_c start, string_view_c end, int32& pos);

int16 p_scan_int16(string_view_c str, string_view_c start, string_view_c end, int32& pos);

int64 p_scan_int64(string_view_c str, string_view_c start, string_view_c end, int32& pos);

long_id p_scan_long_id(string_view_c str, string_view_c start, string_view_c end, int32& pos);

string_view_c p_scan_str(string_view_c str, string_view_c start, string_view_c end, int32& pos);

bool invalid(int32 offset, int32 size);

void store_int64(int32 offset, int64 value);

void add_int64_to(int32 offset, int64 amount);

void resize_chunk(int32 new_size);

string_view_c scan_int64(string_view_c input, string_view_c pattern, int64& output);

string_view_c scan_float64(string_view_c input, string_view_c pattern, float64& output);

bool validate_pk(publickey_c& pk, signature_c& proof);

publickey_c p_scan_pk(string_view_c str, string_view_c start, string_view_c end, int32& pos);

void load_account_chunk(long_id acc_id, long_id local_id);

void load_local_chunk(long_id id);

void store_int16(int32 offset, int16 value);

void store_pk(int32 offset, int32 index, publickey_c& value);

int32 get_chunk_size();

bool verify_by_acc_once(long_id issuer_account, message_c& msg, int16 sigIndex, int32& balance_offset);

bool verify_by_acc(long_id issuer_account, message_c& msg, int16 sigIndex);

int16 virtual_sign(long_id issuer_account, message_c& msg);
} // namespace argc

#endif // ARGENNON_ARGC_FUNCTIONS_H