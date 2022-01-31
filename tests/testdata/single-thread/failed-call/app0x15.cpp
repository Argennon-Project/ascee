#include "argc/types.h"
#include "argc/functions.h"

using namespace argennon;
using namespace ascee;
using namespace argc;

DEF_ARGC_DISPATCHER {
    STRING(dummy, "too long");
    append_str(response, dummy);
    STRING_BUFFER(temp, 7);
    append_str(temp, dummy);
    return HTTP_OK;
}



