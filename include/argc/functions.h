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

#ifndef ASCEE_ARGC_FUNCTIONS_H
#define ASCEE_ARGC_FUNCTIONS_H

#include "types.h"

#ifdef __cplusplus
namespace ascee::argcrt {

extern "C" {
#endif

int64 loadInt64(int32 offset);
int invoke_dispatcher(byte forwarded_gas, std_id_t app_id, string_t request);
void invoke_deferred(byte forwarded_gas, std_id_t app_id, string_t request);
void enter_area();
void exit_area();
void append_str(string_buffer*, string_t);
void append_int64(string_buffer*, int64);
string_buffer* response_buffer();
void clear_buffer(string_buffer* buf);
string_t buf_to_string(const string_buffer* buf);

#ifdef __cplusplus
}

} // namespace argcrt
#endif
#endif // ASCEE_ARGC_FUNCTIONS_H