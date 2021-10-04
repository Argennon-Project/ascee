
#include "argc/types.h"
#include "argc/functions.h"

int dispatcher(string_t request) {
    append_str(getResponseBuffer(), request);
    string_t req = STRING("request from 15");
    invoke_dispatcher(85, 11, req);
    string_t response = STRING(" got in 15");
    append_str(getResponseBuffer(), response);
    return HTTP_OK;
}

