#ifndef ASCEE_APP_LOADER_H
#define ASCEE_APP_LOADER_H

#include <unordered_map>
#include <iostream>

#include "../../include/argc/types.h"

namespace ascee {

class AppLoader {
private:
    static std::unordered_map<std_id_t, dispatcher_ptr_t> dispatchersMap;
public:
    static void init(std_id_t);

    static dispatcher_ptr_t getDispatcher(std_id_t appID);
};

}

#endif // ASCEE_APP_LOADER_H
