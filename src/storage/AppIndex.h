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

#ifndef ARG_CORE_APP_ASA_INDEX_H
#define ARG_CORE_APP_ASA_INDEX_H

#include <vector>
#include "core/info.h"
#include "executor/AppTable.h"
#include "AppLoader.h"

namespace argennon::asa {
/**
 * It should be noted that when an app is updated or a new app is installed, changes can not be seen in the
 * current block, and the updated app can be used in the next block.
 */
class AppIndex {
public:
    AppIndex(AppLoader* loader);

    virtual ~AppIndex();

    /**
     * @note This function is thread-safe.
     * @param sortedAppList
     * @return
     */
    ascee::runtime::AppTable buildAppTable(std::vector<long_id> sortedAppList) const;

    /**
     * Updates the applications included in @p appList in the internal cache, to the state of the provided @p block.
     * @note This function is not thread-safe.
     * @param block
     * @param appList
     */
    void prepareApps(const BlockInfo& block, const std::vector<long_id>& appList);

private:
    std::unordered_map<uint64_t, AppLoader::AppHandle> cache;
    AppLoader* loader;
};

} // namespace argennon::asa
#endif // ARG_CORE_ASA_APP_INDEX_H
