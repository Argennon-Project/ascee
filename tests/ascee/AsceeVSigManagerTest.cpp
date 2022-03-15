
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

#include "gtest/gtest.h"
#include "executor/VirtualSignatureManager.h"

using namespace argennon::ascee::runtime;


TEST(AsceeVSigManagerTest, Simple) {
    VirtualSignatureManager signer({});
    auto sig = signer.sign("Hi all!", 1234);

    EXPECT_FALSE(signer.verify("Hi all!", 12, sig));

    EXPECT_FALSE(signer.verify("Hi al!", 1234, sig));

    EXPECT_TRUE(signer.verify("Hi all!", 1234, sig));

    EXPECT_TRUE(signer.verifyAndInvalidate("Hi all!", 1234, sig));

    EXPECT_FALSE(signer.verifyAndInvalidate("Hi all!", 1234, sig));

    EXPECT_FALSE(signer.verify("Hi all!", 1234, sig));
}


