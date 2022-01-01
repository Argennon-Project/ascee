
#include "argc/types.h"
#include "argc/functions.h"

using namespace ascee;
using namespace ascee::argc;

extern "C"
int dispatcher(string_c request) {
    append_str(response_buffer(), request);
    string_c req = STRING("request from 15");
    invoke_dispatcher(85, 11, req);
    string_c response = STRING(" got in 15");
    append_str(response_buffer(), response);
    return HTTP_OK;
}

