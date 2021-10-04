
#include "argc/types.h"
#include "argc/functions.h"

int dispatcher(string_t request) {
    string_t response = STRING(" is DONE!");
    append_str(getResponseBuffer(), request);
    append_str(getResponseBuffer(), response);
    return HTTP_OK;
}

