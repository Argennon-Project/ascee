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

#ifndef ARGENNON_UTIL_BLOCKING_QUEUE_H
#define ARGENNON_UTIL_BLOCKING_QUEUE_H

#include <mutex>
#include <condition_variable>
#include <queue>

namespace argennon::util {

template<typename T>
class BlockingQueue {
public:
    void enqueue(const T& value) {
        std::unique_lock<std::mutex> lk(queueMutex);

        content.push(value);

        // Manual unlocking is done before notifying, to avoid waking up
        // the waiting thread only to block again (see notify_one for details)
        lk.unlock();
        cv.notify_one();
    }

    T blockingDequeue(bool addProducer) {
        // Wait until queue has items
        std::unique_lock<std::mutex> lk(queueMutex);
        // predicate does not need to acquire any locks, lk does the required locking
        cv.wait(lk, [this] { return !(content.empty() && producerCount > 0); });
        // After the wait, we own the lock.
        if (content.empty()) throw std::underflow_error("empty queue without any producers");

        if (addProducer) ++producerCount;

        T result = content.front();
        content.pop();
        return result;
    }

    /**
     * Use with caution. In many situations instead of this function @p addProducer flag of @p blockingDequeue
     * should be used.
     */
    void addProducer() {
        std::lock_guard<std::mutex> lock(queueMutex);
        ++producerCount;
    }

    void removeProducer() {
        std::unique_lock<std::mutex> lk(queueMutex);
        --producerCount;
        if (producerCount == 0) {
            lk.unlock();
            cv.notify_all();
        }
    }

    bool isEmpty() {
        std::lock_guard<std::mutex> lock(queueMutex);
        return content.empty();
    }

private:
    std::mutex queueMutex;
    std::condition_variable cv;
    std::queue<T> content;
    int producerCount = 0;
};

} // namespace argennon::util
#endif // ARGENNON_UTIL_BLOCKING_QUEUE_H
