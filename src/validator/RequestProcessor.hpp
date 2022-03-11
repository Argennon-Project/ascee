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

#ifndef ARGENNON_AVE_REQUEST_PROCESSOR_H
#define ARGENNON_AVE_REQUEST_PROCESSOR_H

#include <vector>
#include "RequestScheduler.h"

namespace argennon::ave {


class RequestProcessor {
public:
    RequestProcessor(
            asa::ChunkIndex& chunkIndex,
            asa::AppIndex& appIndex,
            int32_fast numOfRequests,
            int workersCount = -1
    ) : scheduler(numOfRequests, chunkIndex, appIndex), numOfRequests(numOfRequests),
        workersCount(workersCount < 1 ? (int) std::thread::hardware_concurrency() * 2 : workersCount) {}

    template<class RequestStream>
    void loadRequests(std::vector<RequestStream> streams) {
        runAll([&](int i) {
            try {
                while (true) scheduler.addRequest(streams.at(i).next());
            } catch (const typename RequestStream::EndOfStream&) {}
        }, streams.size(), workersCount);

        runAll([&](AppRequestIdType requestID) {
            scheduler.finalizeRequest(requestID);
        }, numOfRequests, workersCount);
    }

    void buildDependencyGraph() {
        auto sortedMap = scheduler.sortAccessBlocks(workersCount);

        for (long i = 0; i < sortedMap.size(); ++i) {
            runAll([&, i](long j) {
                auto& chunkList = sortedMap.getValues()[i];
                auto chunkLocalID = chunkList.getKeys()[j];
                auto& blocks = chunkList.getValues()[j];
                scheduler.checkCollisions(full_id(sortedMap.getKeys()[i], chunkLocalID),
                                          std::move(blocks.getKeys()),
                                          std::move(blocks.getValues()));
            }, sortedMap.getValues()[i].size(), workersCount);
        }
    };

    template<class Executor>
    std::vector<ascee::runtime::AppResponse> executeRequests() {
        scheduler.buildExecDag();

        std::vector<ascee::runtime::AppResponse> responseList(numOfRequests);

        std::vector<std::future<void>> pendingTasks;
        pendingTasks.reserve(workersCount);
        for (int i = 0; i < workersCount; ++i) {
            pendingTasks.emplace_back(std::async([&] {
                // each thread must have its own executor
                Executor executor;
                while (auto* request = scheduler.nextRequest()) {
                    responseList[request->id] = executor.executeOne(request);
                    scheduler.submitResult(request->id, responseList[request->id].statusCode);
                }
            }));
        }

        for (auto& pending: pendingTasks) {
            pending.get();
        }

        return responseList;
    }

    /**
     *
     * @param task is a function that accepts a taskID and runs the corresponding task with that id.
     * @param tasksCount
     * @param workersCount
     */
    static
    void runAll(const std::function<void(int64_fast)>& task, int64_fast tasksCount, int workersCount) {
        const auto step = std::max<int64_fast>(tasksCount / workersCount, 1);

        std::vector<std::future<void>> pendingTasks;
        pendingTasks.reserve(workersCount);
        for (int i = 0; i <= workersCount; ++i) {
            pendingTasks.emplace_back(std::async([&, i] {
                for (int64_fast taskID = i * step; taskID < (i + 1) * step && taskID < tasksCount; ++taskID) {
                    task(taskID);
                }
            }));
        }

        for (auto& pending: pendingTasks) {
            // by using get() instead of wait() exceptions will be rethrown here.
            pending.get();
        }
    }

private:
    RequestScheduler scheduler;
    const int32_fast numOfRequests;
    int workersCount;
};

} // namespace argennon::ave
#endif // ARGENNON_AVE_REQUEST_PROCESSOR_H
