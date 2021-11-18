#include "argc/types.h"
#include "argc/functions.h"

using namespace ascee;
using namespace ascee::argc;

extern "C"
int dispatcher(string_t request) {
    string_t dummy = STRING("too long");
    append_str(response_buffer(), dummy);
    STRING_BUFFER(temp, 7);
    append_str(temp, dummy);
    return HTTP_OK;
}



