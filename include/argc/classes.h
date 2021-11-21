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


#ifndef ASCEE_CLASSES_TYPES_H
#define ASCEE_CLASSES_TYPES_H

#include <util/StringBuffer.h>
#include <crypto/Keys.h>
#include "primitives.h"

namespace ascee {

/// argc strings are not null-terminated. However, usually there is a null at the end. `length` is the number of
/// bytes without considering any null bytes at the end.
using string_c = runtime::StringView;
template<int max_size>
using string_buffer_c = runtime::StringBuffer<max_size>;
using signature_c = runtime::Signature;
using publickey_c = runtime::PublicKey;

} // namespace ascee
#endif // ASCEE_CLASSES_TYPES_H
