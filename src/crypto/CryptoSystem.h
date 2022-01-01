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

#ifndef ASCEE_CRYPTO_SYSTEM_H
#define ASCEE_CRYPTO_SYSTEM_H

#include <pbc.h>
#include <openssl/evp.h>
#include <util/StringBuffer.h>
#include "Keys.h"


namespace ascee::runtime {

/// This class is NOT secure and it should only be used for Debugging.
class CryptoSystem {
public:
    CryptoSystem();

    ~CryptoSystem();

    void generateKeyPair(SecretKey& sk, PublicKey& pk);

    Signature sign(const StringView& msg, SecretKey& sk);

    bool verify(const StringView& msg, Signature& sig, PublicKey& pk);

    static int getDigest(const StringView& msg, unsigned char* digest);

private:
    pairing_t pairing{};
    // We shall need several element_t variables to hold the system parameters, keys and other quantities.
    // We declare them here and initialize them in the constructor:
    element_t g{}, h{};
    element_t public_key{}, secret_key{};
    element_t sig_elem{};
    element_t temp1{}, temp2{};
};

} // namespace ascee::runtime
#endif // ASCEE_CRYPTO_SYSTEM_H
