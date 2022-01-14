
#include "argc/types.h"
#include "argc/functions.h"

void foo() {
    foo();
}

using namespace argennon;
using namespace ascee;
using namespace argc;

extern "C"
int dispatcher(string_c request) {
    string_c response = STRING(" OVER FLOW!");
    append_str(response_buffer(), request);
    append_str(response_buffer(), response);
    foo();
    return HTTP_OK;
}

