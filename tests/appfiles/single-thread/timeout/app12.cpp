
#include "argc/types.h"
#include "argc/functions.h"

using namespace ascee;
using namespace ascee::argc;

extern "C"
int dispatcher(string_c request) {
    append_str(response_buffer(), request);
    invoke_dispatcher(85, 10, request);
    string_c response = STRING(" TOO LONG...");
    append_str(response_buffer(), response);
    return HTTP_OK;
}
