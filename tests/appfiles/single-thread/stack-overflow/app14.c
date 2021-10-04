
#include "argc/types.h"
#include "argc/functions.h"

int dispatcher(string_t request) {
    append_str(response_buffer(), request);
    invoke_dispatcher(85, 13, request);
    string_t response = STRING(" OVER FLOW...");
    append_str(response_buffer(), response);
    return HTTP_OK;
}

