
#include "argc/types.h"
#include "argc/functions.h"

int64 fib(int64 n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int dispatcher(string_t request) {
    append_str(response_buffer(), request);
    invoke_dispatcher(85, 13, request);
    string_t response = STRING(" OVER FLOW... fib: ");
    STRING_BUFFER(dummy, 2000 * 1024);
    append_str(response_buffer(), response);
    append_int64(response_buffer(), fib(30));
    return HTTP_OK;
}

