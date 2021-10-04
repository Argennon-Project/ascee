
#include <stdio.h>
#include "argc/types.h"
#include "argc/functions.h"

int dispatcher(string_t request) {
    int dummy[128 * 1024];
    invoke_dispatcher(250, 18, request);
    return HTTP_OK;
}
