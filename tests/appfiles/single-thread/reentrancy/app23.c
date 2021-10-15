
#include <argc/types.h>
#include <argc/functions.h>

int dispatcher(string_t request) {
    return invoke_dispatcher(100, 22, request) + 1;
}

