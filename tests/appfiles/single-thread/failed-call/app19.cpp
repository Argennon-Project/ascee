
#include "argc/types.h"
#include "argc/functions.h"

using namespace ascee;
using namespace ascee::argc;

extern "C"
int dispatcher(string_c request) {
    string_c dummy = STRING("dummy");
    STRING_BUFFER(response, 1024);

    // time out
    append_str(response_buffer(), dummy);
    invoke_dispatcher(40, 10, request);
    append_str(response, buf_to_string(response_buffer()));

    // division zero
    append_str(response_buffer(), dummy);
    invoke_dispatcher(40, 20, request);
    append_str(response, buf_to_string(response_buffer()));

    // self call: App not found
    append_str(response_buffer(), dummy);
    invoke_dispatcher(40, 19, request);
    append_str(response, buf_to_string(response_buffer()));

    // App not declared
    append_str(response_buffer(), dummy);
    invoke_dispatcher(40, 11, request);
    append_str(response, buf_to_string(response_buffer()));

    // segmentation fault
    append_str(response_buffer(), dummy);
    invoke_dispatcher(40, 21, request);
    printf("teeSSSSTTTT->%s", string_c(response_buffer()).data());
    append_str(response, buf_to_string(response_buffer()));


    clear_buffer(response_buffer());
    append_str(response_buffer(), buf_to_string(response));

    string_c end = STRING(" all called...");
    append_str(response_buffer(), end);
    return HTTP_OK;
}
