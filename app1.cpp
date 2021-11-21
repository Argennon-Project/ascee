#include <stdio.h>
#include "argc/types.h"
#include "argc/functions.h"

using namespace ascee;
using namespace ascee::argc;

int fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

extern "C"
int dispatcher(string_c request) {
    printf("ssss%d\n", fib(20));
    string_c response = STRING(" is DONE!");
    append_str(response_buffer(), request);
    append_str(response_buffer(), response);
    return HTTP_OK;
}
