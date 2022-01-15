
#include <stdio.h>
#include "argc/types.h"
#include "argc/functions.h"

using namespace argennon;
using namespace ascee;
using namespace argc;

void f(response_buffer_c& response, int deferred_call) {
    enter_area();

    STRING(req1, "choice: 1");
    STRING(req2, "choice: 2");
    STRING(req3, "choice: 3");

    if (deferred_call) {
        invoke_deferred(23, response, req3);
        // forgetting to call exit_area()...
        return;
    }

    STRING_BUFFER(all, 1024);

    invoke_dispatcher(50, 23, response, req1);
    append_str(all, response);

    invoke_dispatcher(50, 23, response, req2);
    append_str(all, response);

    invoke_dispatcher(50, 23, response, req1);
    append_str(all, response);

    invoke_dispatcher(50, 23, response, req3);
    append_str(all, response);

    clear_buffer(response);
    append_str(response, all);

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


DEF_ARGC_DISPATCHER {
    STRING(pattern, "choice: ");
    string_view_c rest;
    int64 choice = scan_int64(request, pattern, rest);

    if (choice == 1) f(response, 0);
    else if (choice == 2) g();
    else if (choice == 3) return h();
    else if (choice == 4) f(response, 1); // deferred call

    STRING(done, " Done: ");
    append_str(response, done);
    append_int64(response, choice);
    return HTTP_OK;
}
