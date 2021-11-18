#include "argc/types.h"
#include "argc/functions.h"

using namespace ascee;
using namespace ascee::argc;

extern "C"
int dispatcher(string_t request) {
    string_t dummy = STRING("dummy");
    append_str(response_buffer(), dummy);
    int x = 10 / 0;
    return HTTP_OK;
}

