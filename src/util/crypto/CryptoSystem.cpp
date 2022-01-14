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

#include "CryptoSystem.h"

using namespace argennon::util;

#define PARAMS_FILE "../param/a.param"

CryptoSystem::CryptoSystem() {
    // initialize a pairing:
    char param[1024];
    FILE* params = fopen(PARAMS_FILE, "r");
    if (params == nullptr) {
        throw std::runtime_error("param file not found");
    }
    size_t count = fread(param, 1, 1024, params);
    fclose(params);

    if (!count) throw std::runtime_error("input error");

    pairing_init_set_buf(pairing, param, count);

    element_init_G2(g, pairing);
    element_init_G2(public_key, pairing);
    element_init_G1(h, pairing);
    element_init_G1(sig_elem, pairing);
    element_init_GT(temp1, pairing);
    element_init_GT(temp2, pairing);
    element_init_Zr(secret_key, pairing);

    // generate system parameters:
    element_random(g);

    PublicKey pk;
    if (element_length_in_bytes_compressed(public_key) > pk.size()) throw std::length_error("public key length");

    SecretKey sk;
    if (element_length_in_bytes(secret_key) > sk.size()) throw std::length_error("secret key length");

    Signature signature;
    if (element_length_in_bytes_compressed(sig_elem) > signature.size()) throw std::length_error("signature length");
}

CryptoSystem::~CryptoSystem() {
    element_clear(g);
    element_clear(h);
    element_clear(secret_key);
    element_clear(public_key);
    element_clear(sig_elem);
    element_clear(temp1);
    element_clear(temp2);
}

void CryptoSystem::generateKeyPair(SecretKey& sk, PublicKey& pk) {
    // generate a private key:
    element_random(secret_key);

    // and the corresponding public key:
    element_pow_zn(public_key, g, secret_key);

    element_to_bytes(sk.data(), secret_key);
    element_to_bytes_compressed(pk.data(), public_key);
}

int CryptoSystem::getDigest(std::string_view msg, unsigned char* digest) {
    unsigned int len = 0;
    EVP_Digest((void*) msg.data(), msg.length(), digest, &len, EVP_sha3_256(), nullptr);
    return int(len);
}

Signature CryptoSystem::sign(std::string_view msg, SecretKey& sk) {
    element_from_bytes(secret_key, sk.data());

    // When given a message to sign, we first compute its hash, using some standard hash algorithm.
    // Then we map these bytes to an element h of G1:
    unsigned char digest[EVP_MAX_MD_SIZE];
    int digestLen = getDigest(msg, digest);

    element_from_hash(h, digest, digestLen);

    // then sign it:
    element_pow_zn(sig_elem, h, secret_key);

    Signature result;
    element_to_bytes_compressed(result.data(), sig_elem);

    return result;
}

bool CryptoSystem::verify(std::string_view msg, Signature& sig, PublicKey& pk) {
    element_from_bytes_compressed(sig_elem, sig.data());
    element_from_bytes_compressed(public_key, pk.data());

    unsigned char digest[EVP_MAX_MD_SIZE];
    int digestLen = getDigest(msg, digest);

    element_from_hash(h, digest, digestLen);

    // To verify a signature, we compare the outputs of the pairing applied to the signature and system parameter,
    // and the pairing applied to the message hash and public key. If the pairing outputs match then the
    // signature is valid:
    pairing_apply(temp1, sig_elem, g, pairing);
    pairing_apply(temp2, h, public_key, pairing);
    if (!element_cmp(temp1, temp2)) {
        return true;
    } else {
        return false;
    }
}
