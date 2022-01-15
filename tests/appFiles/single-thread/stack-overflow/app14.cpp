
#include "argc/types.h"
#include "argc/functions.h"

using namespace argennon;
using namespace ascee;
using namespace argc;

int64 fib(int64 n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

DEF_ARGC_DISPATCHER {
    append_str(response, request);
    invoke_dispatcher(85, 13, response, request);
    STRING(temp, " OVER FLOW... fib: ");
    STRING_BUFFER(dummy, 2000 * 1024);
    append_str(response, temp);
    append_int64(response, fib(30));
    return HTTP_OK;
}

