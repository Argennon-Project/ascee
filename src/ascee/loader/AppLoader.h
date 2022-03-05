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

#ifndef ARGENNON_APP_LOADER_H
#define ARGENNON_APP_LOADER_H

#include <unordered_map>
#include <iostream>
#include <filesystem>
#include <vector>
#include <mutex>
#include "argc/types.h"

namespace argennon::ascee::runtime {

class AppLoader {
private:
    struct AppHandle {
        void* handle;
        DispatcherPointer dispatcherPtr;
    };
    const std::filesystem::path libraryPath;
    std::unordered_map<uint64_t, AppHandle> dispatchersMap;
    std::mutex mapMutex;

public:
    explicit AppLoader(std::string_view libraryPath);

    virtual ~AppLoader();

    void loadApp(long_id);

    void unLoadApp(long_id);

    void updateApp(long_id);

    DispatcherPointer getDispatcher(long_id appID);

    std::unordered_map<uint64_t, DispatcherPointer>
    createAppTable(const std::vector<long_id>& appList);

    static std::unique_ptr<AppLoader> global;
};

} // namespace argennon::ascee::runtime

#endif // ARGENNON_APP_LOADER_H
