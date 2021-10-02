
#include "AppLoader.h"
#include <dlfcn.h>
#include <thread>
#include <filesystem>
#include <utility>

using namespace ascee;
using namespace std;

unordered_map<std_id_t, dispatcher_ptr_t> AppLoader::dispatchersMap;
filesystem::path libFilesDir;

void AppLoader::loadApp(std_id_t appID) {
    void* handle;
    char* error;
    auto appFile = libFilesDir / ("libapp" + std::to_string(appID) + ".so");

    printf("loading: %s...\n", appFile.c_str());

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
    } catch (const out_of_range& out) {
        try {
            loadApp(appID);
            return getDispatcher(appID);
        } catch (const runtime_error& rte) {
            std::cerr << "err:" << rte.what() << '\n';
            return nullptr;
        }
    }
}

void AppLoader::init(const filesystem::path& p) {
    libFilesDir = p;
}
