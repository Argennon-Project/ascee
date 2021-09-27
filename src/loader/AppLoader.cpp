
#include "AppLoader.h"
#include <dlfcn.h>
#include <thread>

using namespace ascee;
using std::string, std::runtime_error, std::out_of_range;

std::unordered_map<std_id_t, dispatcher_ptr_t> AppLoader::dispatchersMap;

void AppLoader::init(std_id_t appID) {
    void* handle;
    char* error;
    string appFile = string("./libapp") + std::to_string(appID) + string(".so");

    handle = dlopen(appFile.c_str(), RTLD_LAZY);
    if (handle == nullptr) {
        throw runtime_error(dlerror());
    }
    dlerror(); /* Clear any existing error */

    dispatchersMap[appID] = (dispatcher_ptr_t) dlsym(handle, "dispatcher");

    error = dlerror();
    if (error != nullptr) {
        dlclose(handle);
        throw runtime_error(error);
    }
    //todo: we should close these dll files somewhere.
    std::cout << "success!!!!\n";
}

dispatcher_ptr_t AppLoader::getDispatcher(std_id_t appID) {
    try {
        return dispatchersMap.at(appID);
    }
    catch (const out_of_range& e) {
        std::cerr << "err:" << e.what() << '\n';
        return nullptr;
    }
}
