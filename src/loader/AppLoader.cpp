
#include "AppLoader.h"
#include <dlfcn.h>
#include <thread>
#include <filesystem>

using namespace ascee;
using namespace std;

unique_ptr<AppLoader> AppLoader::global;

AppLoader::AppLoader(std::string_view libraryPath) : libraryPath(libraryPath) {}

AppLoader::~AppLoader() {
    scoped_lock<mutex> lock(mapMutex);
    for (const auto& app: dispatchersMap) {
        dlclose(app.second.handle);
    }
    printf("closed\n");
}

void AppLoader::loadApp(std_id_t appID) {
    void* handle;
    char* error;
    auto appFile = libraryPath / ("libapp" + std::to_string(appID) + ".so");

    printf("loading: %s...\n", appFile.c_str());

    handle = dlopen(appFile.c_str(), RTLD_LAZY);
    if (handle == nullptr) {
        throw runtime_error(dlerror());
    }
    dlerror(); /* Clear any existing error */

    auto dispatcher = (dispatcher_ptr_t) dlsym(handle, "dispatcher");

    error = dlerror();
    if (error != nullptr) {
        dlclose(handle);
        throw runtime_error(error);
    }
    scoped_lock<mutex> lock(mapMutex);
    dispatchersMap[appID] = {handle, dispatcher};
}

dispatcher_ptr_t AppLoader::getDispatcher(std_id_t appID) {
    // we should let readers access the map concurrently, but in the current implementation we are not doing so.
    try {
        scoped_lock<mutex> lock(mapMutex);
        return dispatchersMap.at(appID).dispatcher;
    } catch (const out_of_range& out) {
        try {
            loadApp(appID);
        } catch (const runtime_error& rte) {
            std::cerr << "err:" << rte.what() << '\n';
            return nullptr;
        }
        return getDispatcher(appID);
    }
}

unordered_map<std_id_t, dispatcher_ptr_t> AppLoader::createAppTable(const vector<std_id_t>& appList) {
    unordered_map<std_id_t, dispatcher_ptr_t> table(appList.size());
    for (const auto& appID: appList) {
        table[appID] = getDispatcher(appID);
    }
    // hopefully a copy will not happen here!
    return table;
}
