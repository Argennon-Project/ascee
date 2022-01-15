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

int64 loadInt64(int32 offset);

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

int64 scan_int64(string_view_c input, string_view_c pattern, string_view_c& rest);

float64 scan_float64(string_view_c input, string_view_c pattern, string_view_c& rest);

bool verify_by_app(long_id appID, message_c& msg, bool invalidate_msg);

bool verify_by_account(long_id accountID, message_c& msg, signature_c& sig, bool invalidate_msg);

bool verify_by_account(long_id accountID, message_c& msg, bool invalidate_msg);

float64 exact_addf64(float64 a, float64 b);

int dependant_call(long_id app_id, response_buffer_c& response, string_view_c request);

void invoke_deferred(long_id app_id, response_buffer_c& response, string_view_c request);

void revert(string_view_c msg);
} // namespace argc

#endif // ARGENNON_ARGC_FUNCTIONS_H