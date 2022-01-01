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


#ifndef ASCEE_CRYPTO_KEYS_H
#define ASCEE_CRYPTO_KEYS_H

#include <util/StaticArray.h>
#include <argc/primitives.h>


namespace ascee::runtime {

static const int PUBLIC_KEY_SIZE = 65;
static const int SIGNATURE_SIZE = 65;
static const int SECRET_KEY_SIZE = 20;

using SecretKey = StaticArray<byte, SECRET_KEY_SIZE>;
using PublicKey = StaticArray<byte, PUBLIC_KEY_SIZE>;
using Signature = StaticArray<byte, SIGNATURE_SIZE>;

} // namespace ascee::runtime
#endif // ASCEE_CRYPTO_KEYS_H
