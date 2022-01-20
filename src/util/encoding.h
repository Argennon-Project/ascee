// Copyright (c) 2022 aybehrouz <behrouz_ayati@yahoo.com>. All rights
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

#ifndef ARGENNON_ENCODING_H
#define ARGENNON_ENCODING_H


#include <cstddef>

namespace argennon::util {

//Base64url encoding related functions
void base64urlEncode(const void* input, size_t inputLen, char* output, size_t* outputLen = nullptr);

size_t base64urlDecode(const char* input, size_t inputLen, void* output);

size_t base64DecodeLen(size_t binaryLen);

std::string base64urlEncode(const void* input, size_t inputLen);

} // namespace argennon::util
#endif // ARGENNON_ENCODING_H
