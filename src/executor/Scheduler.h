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

#ifndef ASCEE_EXEC_SCHEDULER_H
#define ASCEE_EXEC_SCHEDULER_H

#include <argc/types.h>
#include <util/BlockingQueue.h>
#include <unordered_set>
#include <cassert>
#include <heap/Heap.h>

namespace ascee::runtime {

struct Transaction {
    typedef int_fast32_t IdType;
    IdType id;
    long_id calledAppID;
    string_c request;
    int_fast32_t gas;
    std::vector<long_id> appAccessList;
    std::vector<AppMemAccess> memoryAccessList;
    std::unordered_set<int_fast32_t> stackSizeFailures;
    std::unordered_set<int_fast32_t> cpuTimeFailures;
};

struct TransactionResult {
    Transaction::IdType txID;
    int statusCode;
    std::string response;
};

class DagNode {
public:
    int DecrementInDegree() {
        std::lock_guard<std::mutex> lock(degreeMutex);
        assert(inDegree > 0);
        return --inDegree;
    }

    Transaction* getTransaction() {
        return &tx;
    }

    const std::vector<DagNode*>& adjacentNodes() {
        return adjList;
    }

private:
    Transaction tx;
    std::vector<DagNode*> adjList;
    int inDegree = 0;
    std::mutex degreeMutex;


};

class Scheduler {
public:
    const Transaction* nextTransaction() {
        return zeroQueue.blockingDequeue();
    };

    void submitResult(const TransactionResult& result) {
        // This function is thread-safe
        auto& txNode = nodeIndex.at(result.txID);
        for (const auto& adjacent: txNode->adjacentNodes()) {
            if (adjacent->DecrementInDegree() == 0) {
                zeroQueue.enqueue(adjacent->getTransaction());
            }
        }
        // We don't use nodeIndex.erase(...) because if a rehash happens that would not be thread-safe.
        txNode.reset();
    };

private:
    BlockingQueue<Transaction*> zeroQueue;
    std::unordered_map<Transaction::IdType, std::unique_ptr<DagNode>> nodeIndex;
};

} // namespace ascee::runtime
#endif // ASCEE_EXEC_SCHEDULER_H
