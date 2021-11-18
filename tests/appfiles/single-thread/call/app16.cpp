
#include "argc/types.h"
#include "argc/functions.h"

using namespace ascee;
using namespace ascee::argc;

extern "C"
int dispatcher(string_t request) {
    invoke_dispatcher(85, 555, request);
    string_t response = STRING(" wrong app!");
    append_str(response_buffer(), response);
    return HTTP_OK;
}
