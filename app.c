#include <stdio.h>
#include "argctypes.h"
#include "argcrtl.h"

int fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int dispatcher(void* session, const char* request, char* response) {
    loadInt64(session, 777);
    printf("%s \n fib:%d\n", request, fib(10));
    while (1) {
        printf("%s\n", request);
        for (int i = 0; i < 100000000; ++i) {
        }
    };
}

