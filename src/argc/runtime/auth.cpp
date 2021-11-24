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

#include <argc/types.h>
#include <argc/functions.h>
#include <executor/Executor.h>
#include <crypto/CryptoSystem.h>
#include <util/IdentifierTrie.h>

#define LAST_RESERVED_NONCE 7
#define ARG_APP_ID 0x0100000000000000
#define MASK 0xffffffffffff0000
#define NONCE16_MAX INT16_MAX
#define ARG_BALANCE_BYTES 8

using namespace ascee;
using namespace runtime;

static CryptoSystem cryptoSigner;

static
byte loadNonceChunk(long_id accountID, int32& size) {
    auto& heap = Executor::getSession()->heapModifier;
    heap->loadChunk(accountID & MASK);
    size = heap->getChunkSize();
    return heap->load<byte>(size - 1);
}

static
bool verifyWithNonce(long_id spender, uint32_t nonce, message_c& msg, signature_c& sig, PublicKey& pk) {
    msg.append(",\"spender\":")->append(StringView(std::to_string(spender)));
    msg.append(",\"nonce\":")->append(StringView(std::to_string(nonce)))->append("}");
    return cryptoSigner.verify(StringView(msg), sig, pk);
}

static
bool verifyWithMultiwayNonce(int nonceCount, int32 chunkSize,
                             long_id spender, message_c& msg, signature_c& sig, PublicKey& pk) {
    int32 nonceIndex = chunkSize - 2 * nonceCount + int32(spender % nonceCount) * 2;
    auto nonce = Executor::getSession()->heapModifier->load<uint16_t>(nonceIndex);
    if (nonce >= NONCE16_MAX) return false;

    bool valid = verifyWithNonce(spender, nonce, msg, sig, pk);
    if (valid) Executor::getSession()->heapModifier->store(nonceIndex, nonce + 1);
    return valid;
}

static
bool verifyByAcc(long_id spender, long_id accountID, message_c& msg, signature_c& sig, bool invalidate_msg = true) {
    auto& heap = Executor::getSession()->heapModifier;

    int32 chunkSize;
    byte decisionByte = loadNonceChunk(accountID, chunkSize);

    if (decisionByte > 1 && decisionByte <= LAST_RESERVED_NONCE) {
        return false;
    }

    // decisionByte == 0 means that the owner of the account is an app
    if (decisionByte == 0) {
        auto app = heap->loadIdentifier(gAppTrie, 0);
        return argc::verify_by_app(app, msg, invalidate_msg);
    }

    auto pk = heap->load<PublicKey>(0);

    if (!invalidate_msg) {
        return cryptoSigner.verify(StringView(msg), sig, pk);
    }

    // decisionByte == 1 means that the owner account has 5 nonce values. This improves parallelization for
    // that account. The nonce will be selected based on the id of the spender.
    if (decisionByte == 1) {
        return verifyWithMultiwayNonce(5, chunkSize, spender, msg, sig, pk);
    }

    // decisionByte == 2 means that the owner account has an 11 way nonce value.
    if (decisionByte == 2) {
        return verifyWithMultiwayNonce(11, chunkSize, spender, msg, sig, pk);
    }

    // if decisionByte > LAST_RESERVED_NONCE this account is a normal account which uses a var length nonce.
    if (decisionByte > LAST_RESERVED_NONCE) {
        int32 nonceOffset = pk.size() + ARG_BALANCE_BYTES;
        auto nonce = heap->loadVarUInt(gNonceTrie, nonceOffset);
        bool valid = verifyWithNonce(spender, nonce, msg, sig, pk);
        if (valid) {
            try {
                // If nonce is too big storeVarUInt will throw an exception. Nonce should be much smaller than
                // UINT16_MAX and the cast is safe.
                int len = heap->storeVarUInt(gNonceTrie, nonceOffset, uint16_t(nonce + 1));
                heap->updateChunkSize(nonceOffset + len);
            } catch (const std::overflow_error&) {
                return false;
            }
        }
        return valid;
    }
    // we should not get here.
    return false;
}

bool argc::verify_by_app(long_id appID, message_c& msg, bool invalidate_msg = true) {
    if (invalidate_msg) {
        auto caller = Executor::getSession()->currentCall->appID;
        msg.append(",\"spender\":")->append(StringView(std::to_string(caller)))->append("}");
        return Executor::getSession()->virtualSigner.verifyAndInvalidate(appID, StringView(msg));
    } else {
        return Executor::getSession()->virtualSigner.verify(appID, StringView(msg));
    }
}

bool argc::verify_by_account(long_id accountID, message_c& msg, signature_c& sig, bool invalidate_msg = true) {
    Executor::getSession()->heapModifier->loadContext(ARG_APP_ID);
    auto caller = Executor::getSession()->currentCall->appID;
    bool result = verifyByAcc(caller, accountID, msg, sig, invalidate_msg);
    Executor::getSession()->heapModifier->loadContext(caller);
    return result;
}

bool argc::verify_by_account(long_id accountID, message_c& msg, bool invalidate_msg = true) {
    signature_c dummy;
    return verify_by_account(accountID, msg, dummy, invalidate_msg);
}

