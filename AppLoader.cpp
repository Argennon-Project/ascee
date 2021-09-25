
#include "AppLoader.h"
#include <dlfcn.h>
#include <thread>

unordered_map<std_id_t, dispatcher_ptr_t> AppLoader::dispatchersMap;

void AppLoader::init()
{
    void *handle;
    char *error;
    std_id_t appID = 1;
    string appFile = string("./libapp") + to_string(appID) + string(".so");

    handle = dlopen(appFile.c_str(), RTLD_LAZY);
    if (handle == nullptr)
    {
        throw runtime_error(dlerror());
    }
    dlerror(); /* Clear any existing error */

    dispatchersMap[appID] = (dispatcher_ptr_t)dlsym(handle, "dispatcher");

    error = dlerror();
    if (error != nullptr)
    {
        dlclose(handle);
        throw runtime_error(error);
    }
    //todo: we should close the dll file somewhere.
    cout << "success!!!!\n";
}

dispatcher_ptr_t AppLoader::getDispatcher(std_id_t appID)
{
    try
    {
        return dispatchersMap.at(appID);
    }
    catch (const out_of_range &e)
    {
        cerr << "err:" << e.what() << '\n';
        return nullptr;
    }
}
