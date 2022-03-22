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

#include "subtest.h"
#include <util/crypto/CryptoSystem.h>

using namespace argennon::util;

TEST(UtilCryptoTest, Simple) {
    CryptoSystem signer;

    PublicKey pk;
    SecretKey sk;
    signer.generateKeyPair(sk, pk);

    std::cout << pk.toString() << "\n";

    Signature sig = signer.sign("Helloooo!!", sk);

    std::cout << sig.toBase64() << "\n";

    BENCHMARK(EXPECT_TRUE(signer.verify("Helloooo!!", sig, pk)), 10, "Verifying time:");

    EXPECT_FALSE(signer.verify("Hellooo!!", sig, pk));

    Signature sig2 = signer.sign("Bye", sk);

    EXPECT_TRUE(signer.verify("Bye", sig2, pk));

    EXPECT_FALSE(signer.verify("Bye!", sig2, pk));
}


TEST(UtilCryptoTest, SignSomthing) {
    CryptoSystem signer;
    PublicKey pk;
    SecretKey sk;
    std::string_view msg = "1234 msg msg";

    signer.generateKeyPair(sk, pk);

    std::cout << pk.toString() << "\n";

    Signature sig = signer.sign(msg, sk);

    EXPECT_TRUE(signer.verify(msg, sig, pk));
}
