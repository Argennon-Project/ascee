
#include "argc/types.h"
#include "argc/functions.h"

using namespace argennon;
using namespace ascee;
using namespace argc;

extern "C"
int dispatcher(string_c request) {
    string_c response = STRING(" is DONE!");
    append_str(response_buffer(), request);
    append_str(response_buffer(), response);
    return HTTP_OK;
}

