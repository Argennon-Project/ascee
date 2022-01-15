#include "argc/types.h"
#include "argc/functions.h"

using namespace argennon;
using namespace ascee;
using namespace argc;

DEF_ARGC_DISPATCHER {
    append_str(response, "dummy");
    int x = 10 / 0;
    return HTTP_OK;
}

