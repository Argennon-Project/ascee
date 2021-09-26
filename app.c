#include <stdio.h>
#include "include/argc/types.h"
#include "include/argc/functions.h"

#define String(str) {.content = str, .length = sizeof str}
#define StringBuffer(size) {.buffer = (char[size]){}, .maxSize = size, .end = 0}

int fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int dispatcher(void* session, const char* request, char* response) {
    string_t str = String("Hello!!!  ");
    string_buffer buf = StringBuffer(100);

    append_str(&buf, str);
    append_int64(&buf, 45);
    append_int64(&buf, 123456);
    printf("%s :: %d\n", buf.buffer, buf.end);

    loadInt64(session, 777);
    printf("%s \n fib:%d\n", request, fib(10));
    while (1) {
        printf("%s\n", request);
        for (int i = 0; i < 100000000; ++i) {
        }
    };
}
