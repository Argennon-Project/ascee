#include <stdio.h>
#include "include/argc/types.h"
#include "include/argc/functions.h"

int fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + 1;
}

int dispatcher(void* session, string_t request) {
    enter_area(session);
    string_t str = String("Hello!!!  new");
    string_buffer buf = StringBuffer(100);
    append_str(&buf, str);
    append_int64(&buf, 45);
    append_int64(&buf, 123456);
    printf("%s :: %d\n", buf.buffer, buf.end);

    loadInt64(session, 777);
    printf("fib:%d\n", fib(100000000));
    // for (int i = 0; i < 20000000000; ++i) {}

    //invoke_dispatcher(session, 220, 2, str);
    exit_area(session);
    return HTTP_OK;
}
