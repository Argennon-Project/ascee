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


#ifndef ARGENNON_CRYPTO_KEYS_H
#define ARGENNON_CRYPTO_KEYS_H

#include <util/StaticArray.hpp>
#include <core/primitives.h>


namespace argennon::util {

constexpr int public_key_size_k = 65;
constexpr int signature_size_k = 65;
constexpr int secret_key_size_k = 20;

using SecretKey = StaticArray<byte, secret_key_size_k>;
using PublicKey = StaticArray<byte, public_key_size_k>;
using Signature = StaticArray<byte, signature_size_k>;

} // namespace argennon::ascee::runtime
#endif // ARGENNON_CRYPTO_KEYS_H
