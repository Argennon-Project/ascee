#include "argc/types.h"
#include "argc/functions.h"

int dispatcher(string_t request) {
    string_t dummy = STRING("dummy");
    append_str(response_buffer(), dummy);
    STRING_BUFFER(temp, 5);
    append_str(&temp, dummy);
    return HTTP_OK;
}



