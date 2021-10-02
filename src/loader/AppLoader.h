#ifndef ASCEE_APP_LOADER_H
#define ASCEE_APP_LOADER_H

#include <unordered_map>
#include <iostream>
#include <filesystem>

#include "../../include/argc/types.h"

namespace ascee {

class AppLoader {
private:
    static std::filesystem::path sharedLibsDir;
    static std::unordered_map<std_id_t, dispatcher_ptr_t> dispatchersMap;

    static void loadApp(std_id_t);

public:
    static void init(const std::filesystem::path& p);

    static dispatcher_ptr_t getDispatcher(std_id_t appID);
};

} // namespace ascee

#endif // ASCEE_APP_LOADER_H
