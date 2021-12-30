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

#include <future>
#include "BlockValidator.h"

using namespace node::validator;
using namespace ascee::runtime;
using namespace std;

void waitForAll(const vector<future<void>>& pendingTasks) {
    for (const auto& task: pendingTasks) {
        task.wait();
    }
}

void BlockValidator::conditionalValidate(const Block& current, const Block& previous) {
    blockLoader.loadBlock(current);

    heap::PageCache::ChunkIndex index(cache, blockLoader.getPageAccessList(), previous);

    RequestScheduler scheduler(blockLoader.getNumOfRequests(), index);

    loadRequests(scheduler);

    scheduler.buildDag();

    loadMemoryAccessMap(scheduler);

    executeRequests(scheduler);
}

void BlockValidator::loadRequests(RequestScheduler& scheduler) {
    vector<future<void>> pendingTasks;
    auto numOfRequests = blockLoader.getNumOfRequests();
    pendingTasks.reserve(numOfRequests);
    for (AppRequestRawData::IdType requestID = 0; requestID < numOfRequests; ++requestID) {
        pendingTasks.emplace_back(async([&] {
            scheduler.addRequest(requestID, blockLoader.loadRequest(requestID));
        }));
    }

    waitForAll(pendingTasks);
}

void BlockValidator::loadMemoryAccessMap(RequestScheduler& scheduler) {
    vector<future<void>> pendingTasks;
    for (const auto& appID: blockLoader.getAppAccessList()) {
        for (const auto& chunkID: blockLoader.getChunkAccessList(appID)) {
            pendingTasks.emplace_back(async([&] {
                scheduler.addMemoryAccessList(appID, chunkID, blockLoader.getBlockAccessList(appID, chunkID));
            }));
        }
    }

    waitForAll(pendingTasks);
}

void BlockValidator::executeRequests(RequestScheduler& scheduler, int workersCount) {
    workersCount = workersCount == -1 ? (int) thread::hardware_concurrency() : workersCount;

    thread pool[workersCount];
    for (auto& worker: pool) {
        worker = thread([&] {
            while (auto* request = scheduler.nextRequest()) {
                scheduler.submitResult(executor.executeOne(request));
            }
        });
    }

    for (auto& worker: pool) {
        worker.join();
    }
}
