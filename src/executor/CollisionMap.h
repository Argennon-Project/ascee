// Copyright (c) 2021 aybehrouz <behrouz_ayati@yahoo.com>. All rights reserved.
// This file is part of the C++ implementation of the Argennon smart contract
// Execution Environment (AscEE).
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

#ifndef ASCEE_COLLISION_MAP_H
#define ASCEE_COLLISION_MAP_H

#include <map>
#include <vector>
#include <unordered_set>
#include <argc/primitives.h>
#include "heap/Cache.h"
#include "heap/Modifier.h"

namespace ascee::runtime {

class CollisionMap {
public:
    CollisionMap() {
        heap.cacheBlockData({}, {full_id(10, 100)});
        requestIndex.reserve(10);
        requestIndex.emplace_back(0);
        requestIndex.emplace_back(1);
        requestIndex.emplace_back(2);
    }

    struct AppRequest {
        typedef int_fast32_t IdType;
        IdType id;
        //long_id calledAppID;
        //string_c httpRequest;
        //int_fast32_t gas;
        heap::Modifier modifier{};
        // std::vector<long_id> callList;
        // std::unordered_set<int_fast32_t> stackSizeFailures;
        // std::unordered_set<int_fast32_t> cpuTimeFailures;

        AppRequest(IdType id) : id(id) {}
    };

    struct AccessBlock {
        int32 offset;
        int32 size;
        AppRequest::IdType txID;
        bool writable;
    };

    class DependencyGraph {
    public:
        void registerDependency(AppRequest::IdType u, AppRequest::IdType v) {
            printf("[%d->%d]\n", u, v);
        }
    };

    heap::Cache heap;

    std::vector<AppRequest> requestIndex;
    DependencyGraph graph;

    void findCollisions(long_id appID, long_id chunkID, const std::vector<AccessBlock>& sortedAccessBlocks) {
        auto chunkPtr = heap.getChunk(appID, chunkID);

        for (int i = 0; i < sortedAccessBlocks.size(); ++i) {
            auto& request = requestIndex[sortedAccessBlocks[i].txID];
            auto end = sortedAccessBlocks[i].offset + sortedAccessBlocks[i].size;
            auto writable = sortedAccessBlocks[i].writable;
            auto offset = sortedAccessBlocks[i].offset;

            if (offset == -1) {
                auto newSize = sortedAccessBlocks[i].size;
                request.modifier.defineChunk(appID, chunkID, chunkPtr, newSize, writable);
                end = 0;
            } else {
                request.modifier.defineAccessBlock(appID, chunkID, offset,
                                                   sortedAccessBlocks[i].size, writable);
            }

            for (int j = i + 1; j < sortedAccessBlocks.size(); ++j) {
                if (sortedAccessBlocks[j].offset < end && (writable || sortedAccessBlocks[j].writable)) {
                    graph.registerDependency(sortedAccessBlocks[i].txID, sortedAccessBlocks[j].txID);
                }
            }
        }

    }

private:
};

} // namespace ascee::runtime
#endif // ASCEE_COLLISION_MAP_H
