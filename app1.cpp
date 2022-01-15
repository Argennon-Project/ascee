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

DEF_ARGC_DISPATCHER {
    printf("ssss%d\n", fib(20));
    append_str(response, request);
    append_str(response, " is DONE!");
    return HTTP_OK;
}
