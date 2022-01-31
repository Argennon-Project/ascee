
#include "argc/types.h"
#include "argc/functions.h"

using namespace argennon;
using namespace ascee;
using namespace argc;

DEF_ARGC_DISPATCHER {
    invoke_dispatcher(85, 555, response, request);
    append_str(response, " wrong app!");
    return HTTP_OK;
}
