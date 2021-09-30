#include "include/argc/types.h"
#include "include/argc/functions.h"

#include "stdio.h"

void foo() {
    foo();
}


int dispatcher(string_t request) {
    string_t req = String("Hi!");
    foo();
    return 0;
    // printf("woohoo called!!!\n");
    //while(1);
    //foo();
    //return invoke_dispatcher(250, 1, req);
};
