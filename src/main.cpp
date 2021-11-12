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

#include <pbc.h>

int main(int argc, char const* argv[]) {
    // initialize a pairing:
    pairing_t pairing;
    char param[1024];
    FILE* params = fopen("/home/aybehrouz/pbc-0.5.14/param/a.param", "r");
    if (params == nullptr) {
        printf("here:\n");
    }

    size_t count = fread(param, 1, 1024, params);
    fclose(params);
    if (!count) pbc_die("input error");
    pairing_init_set_buf(pairing, param, count);

    // Later we give pairing parameters to our program on standard input. Any file in the param subdirectory will
    // suffice, for example:

    // $ bls < param/a.param

    // We shall need several element_t variables to hold the system parameters, keys and other quantities.
    // We declare them and initialize them:
    element_t g, h;
    element_t public_key, secret_key;
    element_t sig;
    element_t temp1, temp2;

    element_init_G2(g, pairing);
    element_init_G2(public_key, pairing);
    element_init_G1(h, pairing);
    element_init_G1(sig, pairing);
    element_init_GT(temp1, pairing);
    element_init_GT(temp2, pairing);
    element_init_Zr(secret_key, pairing);

    // generate system parameters:
    element_random(g);

    // generate a private key:
    element_random(secret_key);

    // and the corresponding public key:
    element_pow_zn(public_key, g, secret_key);

    // When given a message to sign, we first compute its hash, using some standard hash algorithm. Many libraries
    // can do this, and this operation does not involve pairings, so PBC does not provide functions for this step.
    // For this example, and our message has already been hashed, possibly using another library. Say the message
    // hash is "ABCDEF" (a 48-bit hash). We map these bytes to an element h of G1:
    element_from_hash(h, (void*) "ABCDEF", 6);

    // then sign it:
    element_pow_zn(sig, h, secret_key);

    // To verify this signature, we compare the outputs of the pairing applied to the signature and system parameter,
    // and the pairing applied to the message hash and public key. If the pairing outputs match then the
    // signature is valid:
    pairing_apply(temp1, sig, g, pairing);
    pairing_apply(temp2, h, public_key, pairing);
    if (!element_cmp(temp1, temp2)) {
        printf("signature verifies\n");
    } else {
        printf("signature does not verify\n");
    }

/*
    char cStr[20] = "abcdefgh";
    string str = string(cStr, 4);
    cout << "this->" << str << endl;
    cStr[0] = 'z';
    cout << "that->" << cStr << endl;
    cout << "this->" << str << endl;
    cout << sizeof "abc" << endl;
    Heap heap;
    Executor e;
    return 0;
    auto* modifier = heap.initSession(2);
    short_id_t chunk = 111;
    modifier->loadContext(2);
    modifier->loadChunk(chunk);
    modifier->saveVersion();
    modifier->store(5, 0x1122334455667788);
    printf("\nread->%lx\n", modifier->load<int64_t>(5));

    modifier->restoreVersion(0);
    int64 xyz = modifier->load<int64_t>(5);
    printf("\nread->%lx\n", xyz);

    delete modifier;

    uint8_t test[256];
    int64_t x = 0x12345678;
    auto* t = (int64_t*) (test);
    t[0] = x;
    for (int i = 0; i < 15; ++i) {
        printf("%x ", test[i]);
    }
    printf("\n%lx", t[0]);

     AppLoader::global = std::make_unique<AppLoader>("");

     Executor e;
     Transaction tr1{1, "req", 4, 1000000000000, {1, 2, 3}};
     /// Transaction tr2{2, "req",5000, {2}};

     std::thread t1(&Executor::startSession, &e, tr1);
     //std::thread t2(&Executor::startSession, &e, tr2);

     t1.join();
     //t2.join();
     return 0;*/
}
