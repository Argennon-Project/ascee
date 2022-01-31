#include "argc/types.h"
#include "argc/functions.h"

using namespace argennon;
using namespace ascee;
using namespace argc;

const static int32 balance_index = 0;
/*
bool create_app_account(long_id address, long_id owner_id) {
    load_chunk_long(address);
    if (!invalid(0,1)) return false;
    resize_chunk(2 + sizeof(pk));????
    store_byte(0 , 8);
    store_pk(2, pk);
    return true;
}
*/
bool update_normal_account(long_id address, publickey_c& pk, signature_c& proof) {
    if (!validate_pk(pk, proof)) return false;
    load_chunk_long(address);
    if (!invalid(0, 1)) return false;
    resize_chunk(2 + sizeof(pk));
    store_byte(0, 8);
    store_pk(2, pk);
    return true;
}

void transfer(long_id from, long_id to, int64 amount, signature_c& sig) {
    //todo: we should check this when we are reading the request and remove this check from here
    if (amount < 0) revert("negative amount");
    // loading balance chunk
    load_chunk_long(from & acc_id_mask_g | 1);

    // checking that the balance chunk exists
    if (invalid(balance_index, sizeof(int64))) revert("invalid sender account or zero balance");
    int64 balance = load_int64(balance_index);
    if (balance < amount) revert("not enough balance");
    balance -= amount;
    store_int64(balance_index, balance);
    if (balance == 0) resize_chunk(balance_index);

    // loading nonce chunk
    load_chunk_long(to & acc_id_mask_g);
    if (invalid(0, sizeof(uint16))) revert("invalid recipient account");

    load_chunk_long(to & acc_id_mask_g | 1);
    if (invalid(balance_index, sizeof(int64))) resize_chunk(balance_index + sizeof(int64));
    add_int64_to(balance_index, amount);

    message_c msg;
    append_str(msg, "{\"to\":");
    append_long_id(msg, to);
    append_str(msg, ",\"amount\":");
    append_int64(msg, amount);
    // "," should not be appended to the end.
    if (!verify_by_acc_once(from, msg, sig)) revert("invalid signature");
}

DEF_ARGC_DISPATCHER {
    int32 position = 0;
    string_view_c method = p_scan_str(request, "", " ", position);
    if (method == "PATCH") {
        long_id account = p_scan_long_id(request, "/balances/", "/", position);
        long_id to = p_scan_long_id(request, "{\"to\":", ",", position);
        int64 amount = p_scan_int64(request, "\"amount\":", ",", position);
        signature_c sig = p_scan_sig(request, R"("sig":")", "\"", position);
        transfer(account, to, amount, sig);
        append_str(response, "success and a good response!");
        return HTTP_OK;
    } else if (method == "PUT") {
        long_id address = p_scan_long_id(request, "/balances/", "/", position);
        publickey_c pk = p_scan_pk(request, R"({"pk":")", "\"", position);
        signature_c proof = p_scan_sig(request, R"("sig":")", "\"", position);
        if (update_normal_account(address, pk, proof)) {
            return HTTP_OK;
        } else {
            return 300;
        }
    }
    return HTTP_OK;
}
