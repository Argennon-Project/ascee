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

#ifndef ARGENNON_CRYPTO_SYSTEM_H
#define ARGENNON_CRYPTO_SYSTEM_H

#include <pbc/pbc.h>
#include <openssl/evp.h>
#include "Keys.h"


namespace argennon::util {

/// This class is NOT secure and it should only be used for Debugging.
class CryptoSystem {
public:
    enum class GroupType {
        G1, G2, GT, Zr
    };

    // We shall need several element_t variables to hold the system parameters, keys and other quantities.
    struct Element {
        element_t e{};

        operator element_t&() { // NOLINT(google-explicit-constructor)
            return e;
        }

        Element(GroupType type, pairing_t pairing);

        ~Element();
    };

    CryptoSystem();

    ~CryptoSystem();

    void generateKeyPair(SecretKey& sk, PublicKey& pk);

    Signature sign(std::string_view msg, SecretKey& sk);

    bool verify(std::string_view msg, Signature& sig, PublicKey& pk);

    static int getDigest(std::string_view msg, unsigned char* digest);

private:
    pairing_t pairing{};
    element_t g{};
};

} // namespace argennon::util
#endif // ARGENNON_CRYPTO_SYSTEM_H
