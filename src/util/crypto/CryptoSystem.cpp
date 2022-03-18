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

#define PARAMS_FILE "resources/param/a.param"
#define G_FILE "resources/param/g.rand"

CryptoSystem::CryptoSystem() {
    // initialize a pairing:
    char param[1024];
    FILE* params = fopen(PARAMS_FILE, "r");
    if (params == nullptr) {
        throw std::runtime_error("param file not found");
    }
    size_type count = fread(param, 1, 1024, params);
    fclose(params);

    if (!count) throw std::runtime_error("input error");

    pairing_init_set_buf(pairing, param, count);

    element_t public_key{}, secret_key{};
    element_t sig_elem{}, h{};

    element_init_G2(g, pairing);
    element_init_G2(public_key, pairing);
    element_init_G1(h, pairing);
    element_init_G1(sig_elem, pairing);

    element_init_Zr(secret_key, pairing);

    // generate system parameters:
    int len = element_length_in_bytes(g);
    uint8_t buf[len + 1];
    FILE* g_file = fopen(G_FILE, "r");
    if (g_file == nullptr) {
        element_random(g);
        g_file = fopen(G_FILE, "w");
        element_to_bytes(buf, g);
        fwrite(buf, 1, len, g_file);
    } else {
        auto n = fread(buf, 1, len, g_file);
        if (n != len || element_from_bytes(g, buf) != len) throw std::runtime_error("Crypto: could not read g");
    }
    fclose(g_file);

    element_printf("generator:%B\n", g);

    PublicKey pk;
    if (element_length_in_bytes_compressed(public_key) > pk.size()) throw std::length_error("public key length");

    SecretKey sk;
    if (element_length_in_bytes(secret_key) > sk.size()) throw std::length_error("secret key length");

    Signature signature;
    if (element_length_in_bytes_compressed(sig_elem) > signature.size()) throw std::length_error("signature length");

    element_clear(h);
    element_clear(secret_key);
    element_clear(public_key);
    element_clear(sig_elem);
}

CryptoSystem::~CryptoSystem() {
    element_clear(g);
}

void CryptoSystem::generateKeyPair(SecretKey& sk, PublicKey& pk) {
    // generate a private key:
    Element secretKey(GroupType::Zr, pairing);
    element_random(secretKey);

    // and the corresponding public key:
    Element publicKey(GroupType::G2, pairing);
    element_pow_zn(publicKey, g, secretKey);

    element_to_bytes(sk.data(), secretKey);
    element_to_bytes_compressed(pk.data(), publicKey);
}

int CryptoSystem::getDigest(std::string_view msg, unsigned char* digest) {
    unsigned int len = 0;
    EVP_Digest((void*) msg.data(), msg.length(), digest, &len, EVP_sha3_256(), nullptr);
    return int(len);
}

Signature CryptoSystem::sign(std::string_view msg, SecretKey& sk) {
    Element secretKey(GroupType::Zr, pairing);
    element_from_bytes(secretKey, sk.data());

    // When given a message to sign, we first compute its hash, using some standard hash algorithm.
    // Then we map these bytes to an element h of G1:
    unsigned char digest[EVP_MAX_MD_SIZE];
    int digestLen = getDigest(msg, digest);
    Element hash(GroupType::G1, pairing), sigElem(GroupType::G1, pairing);
    element_from_hash(hash, digest, digestLen);

    // then sign it:
    element_pow_zn(sigElem, hash, secretKey);

    Signature result;
    element_to_bytes_compressed(result.data(), sigElem);
    return result;
}

bool CryptoSystem::verify(std::string_view msg, Signature& sig, PublicKey& pk) {
    Element publicKey(GroupType::G2, pairing);
    Element hash(GroupType::G1, pairing), sigElem(GroupType::G1, pairing);

    element_from_bytes_compressed(sigElem, sig.data());
    element_from_bytes_compressed(publicKey, pk.data());

    unsigned char digest[EVP_MAX_MD_SIZE];
    int digestLen = getDigest(msg, digest);

    element_from_hash(hash, digest, digestLen);

    // To verify a signature, we compare the outputs of the pairing applied to the signature and system parameter,
    // and the pairing applied to the message hash and public key. If the pairing outputs match then the
    // signature is valid:
    Element temp1(GroupType::GT, pairing), temp2(GroupType::GT, pairing);

    pairing_apply(temp1, sigElem, g, pairing);
    pairing_apply(temp2, hash, publicKey, pairing);

    return !element_cmp(temp1, temp2);
}

CryptoSystem::Element::Element(GroupType type, pairing_s* pairing) {
    switch (type) {
        case GroupType::G1:
            element_init_G1(e, pairing);
            break;
        case GroupType::G2:
            element_init_G2(e, pairing);
            break;
        case GroupType::GT:
            element_init_GT(e, pairing);
            break;
        case GroupType::Zr:
            element_init_Zr(e, pairing);
            break;
    }
}

CryptoSystem::Element::~Element() {
    element_clear(e);
}
