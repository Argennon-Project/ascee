#include "argc/types.h"
#include "argc/functions.h"

using namespace argennon;
using namespace ascee;
using namespace argc;

extern "C"
int dispatcher(string_c request) {
    string_c dummy = STRING("too long");
    append_str(response_buffer(), dummy);
    STRING_BUFFER(temp, 7);
    append_str(temp, dummy);
    return HTTP_OK;
}



