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

#include <gtest/gtest.h>
#include <util/crypto/CryptoSystem.h>

using namespace argennon::util;

TEST(AsceeCryptoTest, Simple) {
    CryptoSystem signer;

    PublicKey pk;
    SecretKey sk;
    signer.generateKeyPair(sk, pk);

    std::cout << pk.toString() << "\n";

    Signature sig = signer.sign("Helloooo!!", sk);

    std::cout << sig.toBase64() << "\n";

    EXPECT_TRUE(signer.verify("Helloooo!!", sig, pk));

    EXPECT_FALSE(signer.verify("Hellooo!!", sig, pk));

    sig = signer.sign("Bye", sk);

    EXPECT_TRUE(signer.verify("Bye", sig, pk));

    EXPECT_FALSE(signer.verify("Bye!", sig, pk));
}


TEST(AsceeCryptoTest, SignSomthing) {
    CryptoSystem signer;
    PublicKey pk;
    SecretKey sk;
    std::string_view msg = R"({"to":0xabc000000000000,"amount":1399,"spender":0x1,"nonce":11})";
    signer.generateKeyPair(sk, pk);

    std::cout << pk.toString() << "\n";

    Signature sig = signer.sign(msg, sk);

    std::cout << sig.toBase64() << "\n";

    std::array<uint8_t, 65> temp = {76, 4, 147, 2, 242, 185, 142, 97, 127, 241, 23, 190, 47, 105, 5, 197, 242, 14, 106,
                                    29, 4, 157, 46, 119, 66, 108, 63, 13, 25, 195, 9, 216, 126, 9, 128, 156, 224, 63,
                                    233, 197, 128, 173, 14, 141, 30, 213, 59, 43, 58, 26, 174, 248, 37, 74, 220, 219,
                                    31, 0, 204, 47, 169, 120, 248, 137, 0};

    PublicKey pk2;
    memcpy(&pk2, temp.data(), 65);

    Signature sg("OcTd6Oa93sNeQpZVoN4sd7BOGGnRxfyDJnuitYpOr_g8dtGcgAX8XH2g7klAD50vhrl299NyEgGEG2FTqIscgwA");

    EXPECT_TRUE(signer.verify(msg, sg, pk2));
}
