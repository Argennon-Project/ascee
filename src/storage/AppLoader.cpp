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
using namespace asa;
using namespace ascee;
using namespace std;


AppLoader::AppLoader(std::string_view libraryPath) : libraryPath(libraryPath) {}


AppLoader::AppHandle AppLoader::load(long_id appID) {
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
    return {handle, 0, dispatcherPtr};
}

void AppLoader::unLoad(AppHandle& handle) {
    if (handle.handle != nullptr) dlclose(handle.handle);
    handle.dispatcherPtr = nullptr;
}
