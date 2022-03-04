// Copyright (c) 2021-2022 aybehrouz <behrouz_ayati@yahoo.com>. All rights
// reserved. This file is part of the C++ implementation of the Argennon smart
// contract Execution Environment (AscEE).
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option) any
// later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License
// for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

#include "AppLoader.h"
#include <dlfcn.h>
#include <thread>
#include <filesystem>

using namespace argennon;
using namespace ascee;
using namespace runtime;
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

void AppLoader::loadApp(long_id appID) {
    void* handle;
    char* error;
    auto appLib = libraryPath / ("libapp" + (string) appID + ".so");
    auto appSrc = libraryPath / ("app" + (string) appID + ".argc");
    string includePath = "-I include -I ../include";

    printf("loading: %s...\n", appLib.c_str());

    handle = dlopen(appLib.c_str(), RTLD_LAZY);
    if (!handle) {
        auto cmd = "java -jar resources/argcc.jar \"" + includePath + "\" " + appSrc.string() + " " + appLib.string();
        printf("%s\n", cmd.c_str());
        auto pipe = popen(cmd.c_str(), "r");
        if (pipe) {
            pclose(pipe);
            handle = dlopen(appLib.c_str(), RTLD_LAZY);
        }
        if (!handle) throw runtime_error(dlerror());
    }

    dlerror(); /* Clear any existing error */

    auto dispatcherPtr = (DispatcherPointer) dlsym(handle, "dispatcher");

    error = dlerror();
    if (error != nullptr) {
        dlclose(handle);
        throw runtime_error(error);
    }
    scoped_lock<mutex> lock(mapMutex);
    dispatchersMap[appID] = {handle, dispatcherPtr};
}

DispatcherPointer AppLoader::getDispatcher(long_id appID) {
    // we should let readers access the map concurrently, but in the current implementation we are not doing so.
    try {
        scoped_lock<mutex> lock(mapMutex);
        return dispatchersMap.at(appID).dispatcherPtr;
    } catch (const out_of_range& out) {
        try {
            loadApp(appID);
        } catch (const runtime_error& rte) {
            std::cerr << rte.what() << '\n';
            return nullptr;
        }
        return getDispatcher(appID);
    }
}

unordered_map<uint64_t, DispatcherPointer> AppLoader::createAppTable(const vector<long_id>& appList) {
    unordered_map<uint64_t, DispatcherPointer> table(appList.size());
    for (const auto& appID: appList) {
        table[appID] = getDispatcher(appID);
    }
    // hopefully a copy will not happen here!
    return table;
}
