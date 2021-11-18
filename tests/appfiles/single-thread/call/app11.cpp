
#include "argc/types.h"
#include "argc/functions.h"

using namespace ascee;
using namespace ascee::argc;

extern "C"
int dispatcher(string_t request) {
    string_t response = STRING(" is DONE!");
    append_str(response_buffer(), request);
    append_str(response_buffer(), response);
    return HTTP_OK;
}

