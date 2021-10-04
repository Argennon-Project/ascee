
#include "argc/types.h"
#include "argc/functions.h"

int dispatcher(string_t request) {
    invoke_dispatcher(85, 555, request);
    string_t response = STRING(" wrong app!");
    append_str(getResponseBuffer(), response);
    return HTTP_OK;
}