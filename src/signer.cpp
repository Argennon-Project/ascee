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

#include <iostream>
#include "util/crypto/CryptoSystem.h"

using namespace argennon::util;
using std::cout;

/// use this executable to issue signatures for tests
int main(int argc, char const* argv[]) {
    CryptoSystem signer;
/*
    PublicKey pk;
    SecretKey sk;
    signer.generateKeyPair(sk, pk);
*/
    SecretKey sk({16, 108, 42, 15, 141, 237, 162, 125, 46, 130, 67, 62, 158, 97, 135, 179, 245, 85, 138, 210});

    PublicKey pk(
            {167, 63, 227, 175, 206, 43, 231, 39, 62, 86, 43, 145, 251, 240, 227, 178, 221, 130, 234, 41, 17, 67, 121,
             119, 77, 0, 95, 153, 38, 130, 216, 239, 80, 89, 85, 0, 151, 119, 0, 128, 34, 109, 35, 97, 213, 164, 90, 32,
             235, 166, 222, 205, 23, 213, 117, 203, 40, 224, 7, 128, 243, 108, 37, 70, 0});
    cout << sk.toString() << "\n";
    cout << pk.toString() << "\n";

    std::string_view msg = R"({"to":0xabc000000000000,"amount":1399,"spender":0x100000000000000,"nonce":11})";

    Signature sig = signer.sign(msg, sk);

    cout << sig.toBase64() << "\n";

    Signature sigTest("XbmMJ1msy7QCnWwZqTwG5xRWCP9-VxmMWgsqfj80N-8XfF0KJQ5J9GCXKw1hgmu9X7a6o6jckpSTJzIMWokM_AA");
    cout << signer.verify(msg, sigTest, pk) << std::endl;

    return 0;
}

