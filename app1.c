#include <stdio.h>
#include "argc/types.h"
#include "argc/functions.h"

int fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int dispatcher(string_t request) {
    STRING_BUFFER(buf, 100);
    int ret = invoke_dispatcher(50, 2, request);
    printf("fib:%d\n", fib(21));
    while (1) {
        for (int i = 0; i < 200000000; ++i) {
        }
        printf("Hey!\n");
    }
    return HTTP_OK;
}
