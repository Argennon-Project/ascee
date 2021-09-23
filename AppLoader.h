#ifndef ASCEE_APP_LOADER_H
#define ASCEE_APP_LOADER_H

#include "argctypes.h"
#include <unordered_map>
#include <iostream>

using namespace std;

class AppLoader
{
private:
    static unordered_map<std_id_t, dispatcher_ptr_t> dispatchersMap;
public:
    static void init();
    static dispatcher_ptr_t getDispatcher(std_id_t appID);
};

#endif //ASCEE_APP_LOADER_H
