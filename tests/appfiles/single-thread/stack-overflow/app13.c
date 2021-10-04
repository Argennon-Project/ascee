
#include "argc/types.h"
#include "argc/functions.h"

void foo() {
    foo();
}

int dispatcher(string_t request) {
    string_t response = STRING(" OVER FLOW!");
    append_str(response_buffer(), request);
    append_str(response_buffer(), response);
    foo();
    return HTTP_OK;
}

