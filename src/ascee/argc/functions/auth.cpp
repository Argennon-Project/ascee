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


using namespace argennon;
using namespace ascee;
using namespace runtime;
using namespace util;
using std::string_view;

constexpr int nonce16_size = int(sizeof(uint16));
constexpr int decision_nonce_size = int(sizeof(uint16));
constexpr uint16 nonce16_max = UINT16_MAX;
constexpr long_id nonce_chunk = 0;

static
void appendNonceToMsg(message_c& msg, long_id spender, uint32_t nonce) {
    msg << ",\"forApp\":" << (std::string) spender;
    msg << ",\"nonce\":" << std::to_string(nonce) << "}";
}

static
void appendSpenderToMsg(message_c& msg, long_id spender) {
    msg << ",\"forApp\":" << (std::string) spender << "}";
}

static
bool verifyWithMultiwayNonce(int nonceCount, int32 nonceIndex,
                             long_id spender, message_c& msg, signature_c& sig,
                             int32& balanceIndex, bool invalidate) {
    auto& heap = Executor::getSession()->heapModifier;

    auto pkIndex = nonceIndex + nonceCount * nonce16_size;
    balanceIndex = pkIndex + public_key_size_k;
    if (msg.size() == 0) return true;

    auto pk = heap.load<PublicKey>(pkIndex);

    if (!invalidate) {
        return Executor::getSession()->cryptoSigner.verify(string_view(msg), sig, pk);
    }

    if (nonceCount > 1) nonceIndex += int32(spender % nonceCount) * nonce16_size;
    auto nonce = heap.load<uint16>(nonceIndex);
    if (nonce >= nonce16_max) return false;

    appendNonceToMsg(msg, spender, nonce);
    bool valid = Executor::getSession()->cryptoSigner.verify(string_view(msg), sig, pk);
    if (valid) heap.store<uint16>(nonceIndex, nonce + 1);

    return valid;
}

class Context {
public:
    Context() : caller(Executor::getSession()->currentCall->appID) {
        Executor::getSession()->heapModifier.loadContext(arg_app_id_g);
    }

    ~Context() {
        Executor::getSession()->heapModifier.loadContext(caller);
    }

    const long_id caller;
};

static
bool verifyByApp(long_id appID, message_c& msg, bool invalidateMsg) {
    if (msg.size() == 0) return true;
    if (!invalidateMsg) {
        return Executor::getSession()->virtualSigner.verify(appID, StringView(msg));
    }
    auto caller = Executor::getSession()->currentCall->appID;
    appendSpenderToMsg(msg, caller);
    return Executor::getSession()->virtualSigner.verifyAndInvalidate(appID, StringView(msg));
}

bool verifyByAccount(long_id accountID, message_c& msg, signature_c& sig, int32& balanceIndex, bool invalidateMsg) {
    Context context;
    auto spender = context.caller;
    auto& heap = Executor::getSession()->heapModifier;

    heap.loadChunk(accountID, nonce_chunk);
    if (!heap.isValid(0, nonce16_size)) return false;

    auto decisionNonce = heap.load<uint16>(0);

    // decisionNonce == 0 means that the owner of the account is an app
    if (decisionNonce == 0) {
        auto app = heap.loadIdentifier(app_trie_g, nonce16_size, 0, &balanceIndex);
        balanceIndex += decision_nonce_size;
        return verifyByApp(app, msg, invalidateMsg);
    }

    // decisionNonce == 1 means that the owner account has 5 nonce values. This improves parallelization for
    // that account. The nonce will be selected based on the id of the spender.
    if (decisionNonce == 1) {
        return verifyWithMultiwayNonce(5, decision_nonce_size, spender, msg, sig, balanceIndex, invalidateMsg);
    }

    // decisionNonce == 2 means that the owner account has an 11 way nonce value.
    if (decisionNonce == 2) {
        return verifyWithMultiwayNonce(11, decision_nonce_size, spender, msg, sig, balanceIndex, invalidateMsg);
    }

    // if decisionNonce > LAST_RESERVED_NONCE this account is a normal account which uses a 2 bytes nonce.
    if (decisionNonce >= nonce_start_g) {
        return verifyWithMultiwayNonce(1, 0, spender, msg, sig, balanceIndex, invalidateMsg);
    }

    // decisionNonce > 2 && decisionNonce <= LAST_RESERVED_NONCE: non-used decision values. These
    // are reserved for later use.
    return false;
}

// C does not have default values or overloading we want to keep this api similar to the real argC api.
bool argc::verify_by_acc_once(long_id account_id, message_c& msg, signature_c& sig, int32& balance_offset) {
    Executor::guardArea();
    auto ret = verifyByAccount(account_id, msg, sig, balance_offset, true);
    Executor::unGuard();
    return ret;
}

bool argc::verify_by_app_once(long_id app_id, message_c& msg) {
    Executor::guardArea();
    auto ret = verifyByApp(app_id, msg, true);
    Executor::unGuard();
    return ret;
}

bool argc::verify_by_acc(long_id account_id, message_c& msg, signature_c& sig) {
    int32 dummy;
    return verifyByAccount(account_id, msg, sig, dummy, false);
}

bool argc::verify_by_app(long_id app_id, message_c& msg) {
    return verifyByApp(app_id, msg, false);
}

bool argc::validate_pk(publickey_c& pk, signature_c& proof) {
    return true;
}

