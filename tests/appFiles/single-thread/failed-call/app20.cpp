#include "argc/types.h"
#include "argc/functions.h"

using namespace argennon;
using namespace ascee;
using namespace argc;

extern "C"
int dispatcher(string_c request) {
    string_c dummy = STRING("dummy");
    append_str(response_buffer(), dummy);
    int x = 10 / 0;
    return HTTP_OK;
}

