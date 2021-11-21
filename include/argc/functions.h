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

#ifndef ASCEE_ARGC_FUNCTIONS_INC
#define ASCEE_ARGC_FUNCTIONS_INC

#include <argc/types.h>

namespace ascee::argc {

bool verify_by_acc(long_id accountID, string_c msg, const signature_c& sig);

bool verify_by_app(long_id appID, string_c msg);

bool verify_once_by_acc(long_id accountID, string_c msg, const signature_c& sig);

bool verify_once_by_app(long_id appID, string_c msg);

int64 loadInt64(int32 offset);

int invoke_dispatcher(byte forwarded_gas, long_id app_id, string_c request);

void invoke_deferred(byte forwarded_gas, long_id app_id, string_c request);

void enter_area();

void exit_area();

float64 safe_addf64(float64 a, float64 b);

float64 truncate_float64(float64 f, int n);

template<int maxSize>
void append_str(string_buffer_c <maxSize>& buf, const string_c& str) {
    buf.append(str);
}

template<int maxSize>
void append_int64(string_buffer_c <maxSize>& buf, int64 i) {
    buf.append(string_c(std::to_string(i)));
}

template<int maxSize>
void append_float64(string_buffer_c <maxSize>& buf, float64 f) {
    buf.append(string_c(std::to_string(f)));
}

template<int maxSize>
void clear_buffer(string_buffer_c <maxSize>& buf) {
    buf.clear();
}

template<int maxSize>
string_c buf_to_string(const string_buffer_c <maxSize>& buf) {
    return string_c(buf);
}

int64 scan_int64(const string_c& input, const string_c& pattern, string_c& rest);

float64 scan_float64(const string_c& input, const string_c& pattern, string_c& rest);

string_buffer_c<RESPONSE_MAX_SIZE>& response_buffer();

} // namespace argc

#endif // ASCEE_ARGC_FUNCTIONS_INC