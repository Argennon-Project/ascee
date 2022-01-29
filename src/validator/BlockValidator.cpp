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

using namespace argennon;
using namespace ave;
using namespace asa;
using namespace ascee::runtime;
using std::vector, std::future;

static
void waitForAll(const vector<future<void>>& pendingTasks) {
    for (const auto& task: pendingTasks) {
        task.wait();
    }
}

static
Digest calculateDigest(vector<AppResponse> responses) {
    return {};
}

static
void runAll(const std::function<void(int64_fast)>& task, int64_fast n, int workersCount) {
    auto step = std::max<int64_fast>(n / workersCount, 1);

    vector<future<void>> pendingTasks;
    pendingTasks.reserve(workersCount);
    for (int i = 0; i < workersCount; ++i) {
        pendingTasks.emplace_back(std::async([&, i] {
            for (int64_fast taskID = i * step; taskID < (i + 1) * step && taskID < n; ++taskID) {
                task(taskID);
            }
        }));
    }

    for (const auto& pending: pendingTasks) {
        pending.wait();
    }
}

/// Conditionally validates a block: valid(current | previous). Returns true when the block is valid and false
/// if the block is not valid.
/// Throwing an exception indicates that due to an internal error checking the validity of the block was not possible.
bool BlockValidator::conditionalValidate(const BlockInfo& current, const BlockInfo& previous) {
    try {
        blockLoader.setCurrentBlock(current);

        ChunkIndex index(
                cache.prepareBlockPages(previous, blockLoader.getPageAccessList(), blockLoader.getMigrationList()),
                blockLoader.getProposedSizeBounds(),
                blockLoader.getNumOfChunks()
        );

        RequestScheduler scheduler(blockLoader.getNumOfRequests(), index);

        loadRequests(scheduler);

        buildDependencyGraph(scheduler);

        return calculateDigest(executeRequests(scheduler)) == blockLoader.getResponseListDigest();
    } catch (const BlockError& err) {
        std::cout << err.message << std::endl;
        return false;
    }
}

void BlockValidator::loadRequests(RequestScheduler& scheduler) {
    auto numOfRequests = blockLoader.getNumOfRequests();

    runAll([&](AppRequestIdType requestID) {
        scheduler.addRequest(blockLoader.loadRequest(requestID));
    }, numOfRequests, workersCount);

    runAll([&](AppRequestIdType requestID) {
        scheduler.finalizeRequest(requestID);
    }, numOfRequests, workersCount);
}

void BlockValidator::buildDependencyGraph(RequestScheduler& scheduler) {
    auto sortedMap = scheduler.sortAccessBlocks(workersCount);

    for (long i = 0; i < sortedMap.size(); ++i) {
        runAll([&, i](long j) {
            const auto& chunkList = sortedMap.getValues()[i];
            auto chunkID = chunkList.getKeys()[j];
            const auto& blocks = chunkList.getValues()[j];
            scheduler.findCollisions(full_id(sortedMap.getKeys()[i], chunkID), blocks.getKeys(), blocks.getValues());
        }, sortedMap.getValues()[i].size(), workersCount);
    }
}

vector<AppResponse> BlockValidator::executeRequests(RequestScheduler& scheduler) {
    scheduler.buildExecDag();

    vector<AppResponse> responseList(blockLoader.getNumOfRequests());

    std::thread pool[workersCount];
    for (auto& worker: pool) {
        worker = std::thread([&] {
            while (auto* request = scheduler.nextRequest()) {
                responseList[request->id] = Executor::executeOne(request);
                scheduler.submitResult(request->id, responseList[request->id].statusCode);
            }
        });
    }

    for (auto& worker: pool) {
        worker.join();
    }

    return responseList;
}

BlockValidator::BlockValidator(
        PageCache& cache,
        BlockLoader& blockLoader,
        int workersCount) : cache(cache), blockLoader(blockLoader) {
    this->workersCount = workersCount < 1 ? (int) std::thread::hardware_concurrency() * 2 : workersCount;
}
