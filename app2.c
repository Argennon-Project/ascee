#include "include/argc/types.h"
#include "include/argc/functions.h"

#include "stdio.h"

int dispatcher(string_t request) {
    string_t req = String("Hi!");
    printf("woohoo called!!!\n");
    return invoke_dispatcher(200, 1, req);
};
