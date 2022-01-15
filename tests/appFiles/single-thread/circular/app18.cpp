
#include "argc/types.h"
#include "argc/functions.h"

using namespace argennon;
using namespace ascee;
using namespace argc;

DEF_ARGC_DISPATCHER {
    int dummy[128 * 1024];
    invoke_dispatcher(250, 17, response, request);
    return HTTP_OK;
}

