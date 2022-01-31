
#include "argc/types.h"
#include "argc/functions.h"

void foo() {
    foo();
}

using namespace argennon;
using namespace ascee;
using namespace argc;

DEF_ARGC_DISPATCHER {
    append_str(response, request);
    append_str(response, " OVER FLOW!");
    foo();
    return HTTP_OK;
}

