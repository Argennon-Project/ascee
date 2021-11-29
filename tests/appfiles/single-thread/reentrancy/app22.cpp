
#include <stdio.h>
#include "argc/types.h"
#include "argc/functions.h"

using namespace ascee;
using namespace ascee::argc;

void f(int deferred_call) {
    enter_area();

    string_c req1 = STRING("choice: 1");
    string_c req2 = STRING("choice: 2");
    string_c req3 = STRING("choice: 3");

    if (deferred_call) {
        invoke_deferred(23, req3);
        // forgetting to call exit_area()...
        return;
    }

    int ret1 = invoke_dispatcher(50, 23, req1);

    int ret2 = invoke_dispatcher(50, 23, req2);

    int ret3 = invoke_dispatcher(50, 23, req1);

    int ret4 = invoke_dispatcher(50, 23, req3);

    string_c space = STRING(" ");
    clear_buffer(response_buffer());
    append_int64(response_buffer(), ret1);
    append_str(response_buffer(), space);

    append_int64(response_buffer(), ret2);
    append_str(response_buffer(), space);

    append_int64(response_buffer(), ret3);
    append_str(response_buffer(), space);

    append_int64(response_buffer(), ret4);
    exit_area();
}

void g() {
    // wrong usage of exit_area()
    exit_area();
}

int h() {
    enter_area();
    printf("h called!\n");
    exit_area();
    return 777;
}


extern "C"
int dispatcher(string_c request) {
    string_c pattern = STRING("choice: ");
    string_c rest;
    int64 choice = scan_int64(request, pattern, rest);

    if (choice == 1) f(0);
    else if (choice == 2) g();
    else if (choice == 3) return h();
    else if (choice == 4) f(1); // deferred call

    string_c done = STRING(" done: ");
    append_str(response_buffer(), done);
    append_int64(response_buffer(), choice);
    return HTTP_OK;
}
