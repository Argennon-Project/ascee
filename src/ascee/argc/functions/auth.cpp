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

#include <argc/types.h>
#include <argc/functions.h>
#include <executor/Executor.h>
#include "util/crypto/CryptoSystem.h"


using namespace argennon;
using namespace ascee;
using namespace runtime;
using namespace util;

constexpr uint16 last_reserved_nonce = 7;
constexpr long_id arg_app_id = 0x0100000000000000;
constexpr long_id nonce_chunk_mask = 0xffffffffffff0000;
constexpr uint16 nonce16_max = UINT16_MAX;

static util::CryptoSystem cryptoSigner;

static
uint16 loadPkChunk(long_id accountID) {
    auto& heap = Executor::getSession()->heapModifier;
    heap.loadChunk(accountID & nonce_chunk_mask);
    return heap.load<uint16>(0);
}

static
void appendNonce(message_c& msg, long_id spender, uint32_t nonce) {
    msg << ",\"spender\":" << std::to_string(spender);
    msg << ",\"nonce\":" << std::to_string(nonce) << "}";
}

static
bool verifyWithMultiwayNonce(int nonceCount, int32 nonceIndex,
                             long_id spender, message_c& msg, signature_c& sig, bool invalidate) {
    auto& heap = Executor::getSession()->heapModifier;
    auto pk = heap.load<PublicKey>(nonceIndex + nonceCount * 2);

    if (!invalidate) {
        return cryptoSigner.verify(StringView(msg), sig, pk);
    }

    if (nonceCount > 1) nonceIndex += int32(spender % nonceCount) * 2;
    auto nonce = heap.load<uint16>(nonceIndex);
    if (nonce >= nonce16_max) return false;

    appendNonce(msg, spender, nonce);
    bool valid = cryptoSigner.verify(StringView(msg), sig, pk);
    if (valid) heap.store(nonceIndex, nonce + 1);

    return valid;
}

class Context {
public:
    class SignalBlocker {
    public:
        SignalBlocker() {
            printf("-->>>>signal\n");
            Executor::blockSignals();
        }

        ~SignalBlocker() {
            Executor::unBlockSignals();
        }
    };

    Context() : caller(Executor::getSession()->currentCall->appID) {
        Executor::getSession()->heapModifier.loadContext(arg_app_id);
    }

    ~Context() {
        Executor::getSession()->heapModifier.loadContext(caller);
    }

    SignalBlocker sig;
    const long_id caller;
};

static
bool verifyByApp(long_id appID, message_c& msg, bool invalidate_msg = true) {
    if (!invalidate_msg) {
        return Executor::getSession()->virtualSigner.verify(appID, StringView(msg));
    }
    auto caller = Executor::getSession()->currentCall->appID;
    msg << ",\"spender\":" << std::to_string(caller) << "}";
    return Executor::getSession()->virtualSigner.verifyAndInvalidate(appID, StringView(msg));
}

bool argc::verify_by_account(long_id accountID, message_c& msg, signature_c& sig, bool invalidate_msg = true) {
    Context context;
    auto spender = context.caller;
    auto& heap = Executor::getSession()->heapModifier;

    auto decisionNonce = loadPkChunk(accountID);

    // decisionNonce == 0 means that the owner of the account is an app
    if (decisionNonce == 0) {
        auto app = heap.loadIdentifier(gAppTrie, 2);
        return verifyByApp(app, msg, invalidate_msg);
    }

    // decisionNonce == 1 means that the owner account has 5 nonce values. This improves parallelization for
    // that account. The nonce will be selected based on the id of the spender.
    if (decisionNonce == 1) {
        return verifyWithMultiwayNonce(5, 2, spender, msg, sig, invalidate_msg);
    }

    // decisionNonce == 2 means that the owner account has an 11 way nonce value.
    if (decisionNonce == 2) {
        return verifyWithMultiwayNonce(11, 2, spender, msg, sig, invalidate_msg);
    }

    // if decisionNonce > LAST_RESERVED_NONCE this account is a normal account which uses a 2 bytes nonce.
    if (decisionNonce > last_reserved_nonce) {
        return verifyWithMultiwayNonce(1, 0, spender, msg, sig, invalidate_msg);
    }

    // decisionNonce > 2 && decisionNonce <= LAST_RESERVED_NONCE: non-used decision values. These
    // are reserved for later use.
    return false;
}

bool argc::verify_by_app(long_id appID, message_c& msg, bool invalidate_msg = true) {
    Context::SignalBlocker blocker;
    return verifyByApp(appID, msg, invalidate_msg);
}
