
#include "argc/types.h"
#include "argc/functions.h"

int dispatcher(string_t request) {
    append_str(getResponseBuffer(), request);
    invoke_dispatcher(85, 10, request);
    string_t response = STRING(" TOO LONG...");
    append_str(getResponseBuffer(), response);
    return HTTP_OK;
}
