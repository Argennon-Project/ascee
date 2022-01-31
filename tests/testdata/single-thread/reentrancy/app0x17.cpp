
#include <argc/types.h>
#include <argc/functions.h>

using namespace argennon;
using namespace ascee;
using namespace argc;

DEF_ARGC_DISPATCHER {
    clear_buffer(response);
    int ret = invoke_dispatcher(100, 22, response, request);
    if (ret == 277) revert("deferred success!");
    return 200;
}

