
#include "argc/types.h"
#include "argc/functions.h"

using namespace argennon;
using namespace ascee;
using namespace argc;

DEF_ARGC_DISPATCHER {
    append_str(response, request);
    invoke_dispatcher(85, 10, response, request);
    append_str(response, " TOO LONG...");
    return HTTP_OK;
}
