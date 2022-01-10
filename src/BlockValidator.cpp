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

#include <future>
#include "BlockValidator.h"

#define RUN_TASK(lambda) std::async(lambda)

using namespace node::validator;
using namespace ascee::runtime;
using namespace ascee;
using std::vector, std::future;

void waitForAll(const vector<future<void>>& pendingTasks) {
    for (const auto& task: pendingTasks) {
        task.wait();
    }
}

/// Conditionally validates a block: valid(current | previous). Returns true when the block is valid and false
/// if the block is not valid.
/// Throwing an exception indicates that due to an internal error checking the validity of the block was not possible.
bool BlockValidator::conditionalValidate(const BlockHeader& current, const BlockHeader& previous) {
    try {
        blockLoader.setCurrentBlock(current);

        heap::PageCache::ChunkIndex index(
                cache,
                previous,
                blockLoader.getPageAccessList(),
                blockLoader.getProposedSizeBounds(),
                blockLoader.getNumOfChunks()
        );

        RequestScheduler scheduler(blockLoader.getNumOfRequests(), index);

        loadRequests(scheduler);

        buildDependencyGraph(scheduler);

        executeRequests(scheduler);
    } catch (const ascee::BlockError& err) {
        std::cout << err.message << std::endl;
        return false;
    }
    return true;
}

void BlockValidator::loadRequests(RequestScheduler& scheduler) {
    auto numOfRequests = blockLoader.getNumOfRequests();

    vector<future<void>> pendingTasks;
    pendingTasks.reserve(numOfRequests);
    for (AppRequestIdType requestID = 0; requestID < numOfRequests; ++requestID) {
        pendingTasks.emplace_back(RUN_TASK([&] {
            scheduler.addRequest(blockLoader.loadRequest(requestID));
        }));
    }

    waitForAll(pendingTasks);

    pendingTasks.clear();
    for (AppRequestIdType requestID = 0; requestID < numOfRequests; ++requestID) {
        pendingTasks.emplace_back(RUN_TASK([&] {
            scheduler.finalizeRequest(requestID);
        }));
    }

    waitForAll(pendingTasks);
}

void BlockValidator::buildDependencyGraph(RequestScheduler& scheduler) {
    auto sortedMap = scheduler.sortAccessBlocks();

    vector<future<void>> pendingTasks;
    for (long i = 0; i < sortedMap.size(); ++i) {
        auto appID = sortedMap.getKeys()[i];
        const auto& chunkList = sortedMap.getValues()[i];
        for (long j = 0; j < chunkList.size(); ++j) {
            auto chunkID = chunkList.getKeys()[j];
            const auto& blocks = chunkList.getValues()[j];
            pendingTasks.emplace_back(RUN_TASK([&] {
                scheduler.findCollisions(full_id(appID, chunkID), blocks.getKeys(), blocks.getValues());
            }));
        }
    }

    waitForAll(pendingTasks);
}

void BlockValidator::executeRequests(RequestScheduler& scheduler) {
    scheduler.buildExecDag();

    std::thread pool[workersCount];
    for (auto& worker: pool) {
        worker = std::thread([&] {
            while (auto* request = scheduler.nextRequest()) {
                scheduler.submitResult(executor.executeOne(request));
            }
        });
    }

    for (auto& worker: pool) {
        worker.join();
    }
}

BlockValidator::BlockValidator(
        heap::PageCache& cache,
        BlockLoader& blockLoader,
        int workersCount) : cache(cache), blockLoader(blockLoader) {
    this->workersCount = workersCount == -1 ? (int) std::thread::hardware_concurrency() : workersCount;
}
