
#include "argc/types.h"
#include "argc/functions.h"

using namespace argennon;
using namespace ascee;
using namespace argc;

extern "C"
int dispatcher(string_c request) {
    invoke_dispatcher(85, 555, request);
    string_c response = STRING(" wrong app!");
    append_str(response_buffer(), response);
    return HTTP_OK;
}
