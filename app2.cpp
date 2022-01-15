#include <argc/functions.h>
#include <argc/types.h>


#include "stdio.h"

using namespace argennon;
using namespace ascee;
using namespace argc;


void foo() {
    foo();
}

DEF_ARGC_DISPATCHER {
    int ret = invoke_dispatcher(255, 1, response, request);
    /*  string_t req = STRING("Hi!");
      foo();
      return 0;*/
    // printf("woohoo called!!!\n");
    //while(1);
    //foo();
    //return invoke_dispatcher(250, 1, req);
    return ret;
};
