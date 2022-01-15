
#include "argc/types.h"
#include "argc/functions.h"

using namespace argennon;
using namespace ascee;
using namespace argc;

DEF_ARGC_DISPATCHER {
    clear_buffer(response);
    append_str(response, request);
    append_str(response, " is DONE!");
    return HTTP_OK;
}

