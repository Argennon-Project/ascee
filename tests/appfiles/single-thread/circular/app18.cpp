
#include "argc/types.h"
#include "argc/functions.h"

using namespace ascee;
using namespace ascee::argc;

extern "C"
int dispatcher(string_c request) {
    int dummy[128 * 1024];
    invoke_dispatcher(250, 17, request);
    return HTTP_OK;
}

