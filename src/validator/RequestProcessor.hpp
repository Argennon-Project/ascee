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
#include "BlockLoader.h"

namespace argennon::ave {


class RequestProcessor {
public:
    RequestProcessor(
            asa::ChunkIndex& index,
            int32_fast numOfRequests,
            int workersCount = -1
    ) : scheduler(numOfRequests, index), numOfRequests(numOfRequests), workersCount(workersCount) {
        this->workersCount = workersCount < 1 ? (int) std::thread::hardware_concurrency() * 2 : workersCount;
    }

    template<class RequestStream>
    void loadRequests(std::vector<RequestStream> streams) {
        runAll([&](int i) {
            try {
                while (true) scheduler.addRequest(streams.at(i).next());
            } catch (const typename RequestStream::EndOfStream&) {}
        }, numOfRequests, workersCount);

        runAll([&](AppRequestIdType requestID) {
            scheduler.finalizeRequest(requestID);
        }, numOfRequests, workersCount);

        std::cout << (std::string) scheduler << "\n";
    }

    void buildDependencyGraph() {
        auto sortedMap = scheduler.sortAccessBlocks(workersCount);

        for (long i = 0; i < sortedMap.size(); ++i) {
            runAll([&, i](long j) {
                const auto& chunkList = sortedMap.getValues()[i];
                auto chunkID = chunkList.getKeys()[j];
                const auto& blocks = chunkList.getValues()[j];
                scheduler.findCollisions(full_id(sortedMap.getKeys()[i], chunkID), blocks.getKeys(),
                                         blocks.getValues());
            }, sortedMap.getValues()[i].size(), workersCount);
        }
    };

    template<class Executor>
    std::vector<ascee::runtime::AppResponse> executeRequests(const Executor& executor) {
        scheduler.buildExecDag();

        std::vector<ascee::runtime::AppResponse> responseList(numOfRequests);

        std::thread pool[workersCount];
        for (auto& worker: pool) {
            worker = std::thread([&] {
                while (auto* request = scheduler.nextRequest()) {
                    responseList[request->id] = executor.executeOne(request);
                    scheduler.submitResult(request->id, responseList[request->id].statusCode);
                }
            });
        }

        for (auto& worker: pool) {
            worker.join();
        }

        return responseList;
    }

    static
    void runAll(const std::function<void(int64_fast taskIndex)

    >& task,
    int64_fast n,
    int workersCount
    ) {
        const auto step = std::max<int64_fast>(n / workersCount, 1);

        std::vector<std::future<void>> pendingTasks;
        pendingTasks.reserve(workersCount);
        for (int i = 0; i <= workersCount; ++i) {
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

private:
    RequestScheduler scheduler;
    int32_fast numOfRequests;
    int workersCount = -1;
};

} // namespace argennon::ave
#endif // ARGENNON_AVE_REQUEST_PROCESSOR_H
