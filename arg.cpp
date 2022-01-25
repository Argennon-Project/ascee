#include <cstdio>
#include "argc/types.h"
#include "argc/functions.h"
#include "ascee/executor/Executor.h"

using namespace argennon;
using namespace ascee;
using namespace argc;

const static long_id account_mask = 0xffffffffffff0000;
const static int32 balance_index = 0;

void transfer(long_id from, long_id to, int64 amount, signature_c& sig) {
    //todo: we should check this when we are reading the request and remove this check from here
    if (amount < 0) revert("negative amount");

    // loading balance chunk
    load_chunk_long(from & account_mask | 1);

    // checking that the balance chunk exists
    if (invalid(balance_index, sizeof(int64))) revert("invalid sender account or zero balance");
    int64 balance = load_int64(balance_index);
    if (balance < amount) revert("not enough balance");
    balance -= amount;
    store_int64(balance_index, balance);
    if (balance == 0) resize_chunk(balance_index);

    // loading nonce chunk
    load_chunk_long(to & account_mask);
    if (invalid(0, sizeof(uint16))) revert("invalid recipient account");

    load_chunk_long(to & account_mask | 1);
    if (invalid(balance_index, sizeof(int64))) resize_chunk(balance_index + sizeof(int64));
    add_int64_to(balance_index, amount);

    message_c msg;
    append_str(msg, "{\"amount\":");
    append_int64(msg, amount);
    append_str(msg, ",");
    // if (!verify_by_acc_once(from, msg, sig)) revert("invalid signature");
}

DEF_ARGC_DISPATCHER {
    int32 position = 0;
    string_view_c method = str_match(request, "", " ", position);
    if (method == "PATCH") {
        long_id account = long_id_match(request, "/balances/", "/", position);
        int64 content_len = int64_match_pattern(request, "Content-Length:", "\r\n\r\n", position);
        long_id to = long_id_match(request, "{\"to\":", ",", position);
        int64 amount = int64_match_pattern(request, "\"amount\":", ",", position);
        signature_c sig = sig_match_pattern(request, R"("sig":")", "\"", position);
        transfer(account, to, amount, sig);
        append_str(response, "success and a good response!");
        return HTTP_OK;
    }
    return HTTP_OK;
}
