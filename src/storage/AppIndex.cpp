// Copyright (c) 2022 aybehrouz <behrouz_ayati@yahoo.com>. All rights
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

#include "AppIndex.h"
#include "core/info.h"

using namespace argennon;
using namespace asa;
using namespace ascee::runtime;
using std::vector, std::pair, std::future;

// this function must be thread-safe
AppTable AppIndex::buildAppTable(vector<long_id> sortedAppList) const {
    vector<ascee::DispatcherPointer> dispatchers;
    dispatchers.reserve(sortedAppList.size());
    for (const auto& appID: sortedAppList) {
        try {
            dispatchers.push_back(cache.at(appID).dispatcherPtr);
        } catch (const std::out_of_range&) {
            throw BlockError("app:" + (std::string) appID + " was not declared in the block's call list");
        }
    }
    return AppTable(util::OrderedStaticMap(std::move(sortedAppList), std::move(dispatchers)));
}

void AppIndex::prepareApps(const BlockInfo& block, const vector<long_id>& appList) {
    // Current implementation is not complete. In the complete implementation this function should also check that
    // the locally stored app is up-to-date and if it isn't it should download the up-to-date version from a PV-DB
    // server.
    vector<pair<long_id, future<AppLoader::AppHandle>>> pendingHandles;
    for (const auto& appID: appList) {
        if (!cache.contains(appID)) {
            pendingHandles.emplace_back(appID, std::async(&AppLoader::load, loader, appID));
        }
    }
    for (auto& pending: pendingHandles) {
        try {
            cache.try_emplace(pending.first, pending.second.get());
        } catch (const std::runtime_error& err) {
            std::cerr << err.what() << '\n';
            cache.try_emplace(pending.first, AppLoader::AppHandle{nullptr, 0, nullptr});
        }
    }
}

AppIndex::AppIndex(AppLoader* loader) : loader(loader) {}

AppIndex::~AppIndex() {
    for (auto& app: cache) {
        loader->unLoad(app.second);
    }
    printf("closed\n");
}
