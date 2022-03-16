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
using std::string_view, std::string;

constexpr uint16 nonce16_max = UINT16_MAX;

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
bool verifyWithMultiwayNonce(int nonceCount,
                             long_id spender, message_c& msg, int16 sigIndex, long_id issuerID,
                             int32& balanceIndex, bool invalidate) {
    auto nonceIndex = 0;
    auto& heap = Executor::getSession()->heapModifier;

    balanceIndex = nonceCount * nonce16_size_g + public_key_size_k;
    if (msg.size() == 0) return true;

    if (!invalidate) {
        return Executor::getSession()->sigManager.verify(string_view(msg), issuerID, sigIndex);
    }

    if (nonceCount > 1) nonceIndex += int32(spender % nonceCount) * nonce16_size_g;
    auto nonce = heap.load<uint16>(nonceIndex);
    if (nonce >= nonce16_max) return false;

    appendNonceToMsg(msg, spender, nonce);
    bool valid = Executor::getSession()->sigManager.verify(string_view(msg), issuerID, sigIndex);
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
bool verifyByApp(long_id issuerID, message_c& msg, int16 sigIndex, bool invalidateMsg) {
    if (msg.size() == 0) return true;
    if (!invalidateMsg) {
        return Executor::getSession()->sigManager.verify(string_view(msg), issuerID, sigIndex);
    }
    auto caller = Executor::getSession()->currentCall->appID;
    appendSpenderToMsg(msg, caller);
    return Executor::getSession()->sigManager.verifyAndInvalidate(string_view(msg), issuerID, sigIndex);
}

bool verifyByAccount(long_id accountID, message_c& msg, int16 sigIndex, int32& balanceIndex, bool invalidateMsg) {
    Context context;
    auto spender = context.caller;
    auto& heap = Executor::getSession()->heapModifier;

    heap.loadChunk(accountID, nonce_chunk_local_id_g);
    if (!heap.isValid(0, nonce16_size_g)) return false;

    auto decisionNonce = heap.load<uint16>(0);

    // decisionNonce == 0 means that the owner of the account is an app
    if (decisionNonce == 0) {
        balanceIndex = decision_nonce_size_g;
        return verifyByApp(accountID, msg, sigIndex, invalidateMsg);
    }

    // decisionNonce == 1 means that the owner account has 5 nonce values. This improves parallelization for
    // that account. The nonce will be selected based on the id of the spender.
    if (decisionNonce == 1) {
        return verifyWithMultiwayNonce(5, spender, msg, sigIndex, accountID, balanceIndex, invalidateMsg);
    }

    // decisionNonce == 2 means that the owner account has an 11 way nonce value.
    if (decisionNonce == 2) {
        return verifyWithMultiwayNonce(11, spender, msg, sigIndex, accountID, balanceIndex, invalidateMsg);
    }

    // if decisionNonce > LAST_RESERVED_NONCE this account is a normal account which uses a 2 bytes nonce.
    if (decisionNonce >= min_nonce_g) {
        return verifyWithMultiwayNonce(1, spender, msg, sigIndex, accountID, balanceIndex, invalidateMsg);
    }

    // decisionNonce > 2 && decisionNonce <= LAST_RESERVED_NONCE: non-used decision values. These
    // are reserved for later use.
    return false;
}

int16 argc::virtual_sign(long_id issuer_account, message_c& msg) {
    Executor::guardArea();
    Context context;
    auto signer = context.caller;
    auto& heap = Executor::getSession()->heapModifier;

    heap.loadChunk(issuer_account, nonce_chunk_local_id_g);
    if (!heap.isValid(0, nonce16_size_g)) {
        throw Executor::Error((string) issuer_account + " is not valid",
                              StatusCode::internal_error, "virtual_sign");
    }

    auto decisionNonce = heap.load<uint16>(0);

    // decisionNonce == 0 means that the owner of the account is an app
    if (decisionNonce != 0) {
        throw Executor::Error((std::string) issuer_account + " does not belong to an app",
                              StatusCode::internal_error, "virtual_sign");
    }

    long_id owner = heap.loadIdentifier(app_trie_g, nonce16_size_g, 0);
    if (signer != owner) {
        throw Executor::Error((string) owner + " is not owner of:" + (string) issuer_account,
                              StatusCode::internal_error, "virtual_sign");
    }

    int16 ret = (int16) Executor::getSession()->sigManager.sign(string(msg), issuer_account);

    Executor::unGuard();
    return ret;
}

// C does not have default values or overloading we want to keep this api similar to the real argC api.
bool argc::verify_by_acc_once(long_id account_id, message_c& msg, int16 sigIndex, int32& balance_offset) {
    Executor::guardArea();
    auto ret = verifyByAccount(account_id, msg, sigIndex, balance_offset, true);
    Executor::unGuard();
    return ret;
}

bool argc::verify_by_app_once(long_id account_id, message_c& msg, int16 sigIndex) {
    Executor::guardArea();
    auto ret = verifyByApp(account_id, msg, sigIndex, true);
    Executor::unGuard();
    return ret;
}

bool argc::verify_by_acc(long_id account_id, message_c& msg, int16 sigIndex) {
    int32 dummy;
    return verifyByAccount(account_id, msg, sigIndex, dummy, false);
}

bool argc::verify_by_app(long_id account_id, message_c& msg, int16 sigIndex) {
    return verifyByApp(account_id, msg, sigIndex, false);
}

bool argc::validate_pk(publickey_c& pk, signature_c& proof) {
    return true;
}

