#include <stdio.h>
#include "argc/types.h"
#include "argc/functions.h"

using namespace argennon;
using namespace ascee;
using namespace argc;

int fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

void transfer(long_id from, long_id to, int64 amount, signature_c& sig) {
    printf("-->%lx %lx %ld %s\n", from, to, amount, sig.toString().c_str());
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
    }
    return HTTP_OK;
}
