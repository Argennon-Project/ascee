
#include <argc/types.h>
#include <argc/functions.h>

using namespace ascee;
using namespace ascee::argc;

extern "C"
int dispatcher(string_c request) {
    return invoke_dispatcher(100, 22, request) + 1;
}

