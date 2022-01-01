
#include <argc/types.h>
#include <argc/functions.h>

using namespace ascee;
using namespace ascee::argc;

extern "C"
int dispatcher(string_c request) {
    int ret = invoke_dispatcher(100, 22, request);
    if (ret == 277) revert("deferred success!");
    return 200;
}

