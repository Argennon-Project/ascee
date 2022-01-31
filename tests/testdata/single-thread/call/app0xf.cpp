
#include "argc/types.h"
#include "argc/functions.h"

using namespace argennon;
using namespace ascee;
using namespace argc;

DEF_ARGC_DISPATCHER {
    append_str(response, request);
    STRING(req, "request from 15");
    invoke_dispatcher(85, 11, response, req);
    STRING(temp, " got in 15");
    append_str(response, temp);
    return HTTP_OK;
}

