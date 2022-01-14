
#include <stdio.h>
#include "argc/types.h"
#include "argc/functions.h"

using namespace argennon;
using namespace ascee;
using namespace argc;

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

    STRING_BUFFER(temp, 1024);
    invoke_dispatcher(50, 23, req1);
    append_str(temp, response_buffer());

    invoke_dispatcher(50, 23, req2);
    append_str(temp, response_buffer());

    invoke_dispatcher(50, 23, req1);
    append_str(temp, response_buffer());

    invoke_dispatcher(50, 23, req3);
    append_str(temp, response_buffer());

    clear_buffer(response_buffer());
    append_str(response_buffer(), temp);

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
    return 277;
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

    string_c done = STRING(" Done: ");
    append_str(response_buffer(), done);
    append_int64(response_buffer(), choice);
    return HTTP_OK;
}
