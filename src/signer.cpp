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

    cout << sk.toString() << "\n";
    cout << pk.toString() << "\n";
*/

    SecretKey sk1({16, 108, 42, 15, 141, 237, 162, 125, 46, 130, 67, 62, 158, 97, 135, 179, 245, 85, 138, 210});
    PublicKey pk1(
            {167, 63, 227, 175, 206, 43, 231, 39, 62, 86, 43, 145, 251, 240, 227, 178, 221, 130, 234, 41, 17, 67, 121,
             119, 77, 0, 95, 153, 38, 130, 216, 239, 80, 89, 85, 0, 151, 119, 0, 128, 34, 109, 35, 97, 213, 164, 90, 32,
             235, 166, 222, 205, 23, 213, 117, 203, 40, 224, 7, 128, 243, 108, 37, 70, 0});

    SecretKey sk2({83, 64, 92, 172, 129, 243, 237, 244, 8, 118, 126, 226, 78, 130, 117, 165, 239, 61, 219, 211});
    PublicKey pk2(
            {114, 22, 249, 146, 208, 168, 123, 133, 195, 106, 1, 214, 151, 30, 120, 212, 68, 137, 15, 44, 196, 26, 36,
             117, 77, 117, 168, 34, 255, 48, 33, 2, 74, 202, 10, 109, 73, 202, 157, 101, 14, 123, 31, 252, 170, 225, 59,
             19, 145, 115, 30, 178, 169, 157, 92, 103, 183, 224, 249, 75, 147, 36, 18, 64, 1});


    std::string_view msg = R"({"to":0xaabc000000000000,"amount":556677,"forApp":0x100000000000000,"nonce":255})";

    Signature sig = signer.sign(msg, sk2);

    cout << "sig: " << sig.toBase64() << "\n";
    cout << "pk: " << pk2.toBase64() << "\n";

    Signature sigTest("MlzLSpbWl9kGUjH3FnMV-oD_ykknkDHi8ayBOlQGIl83icEFgeNxhtO58LU1QFVwWX4-cyKpclDid1W88RzdHgA");
    cout << signer.verify(msg, sigTest, pk2) << std::endl;

    return 0;
}

