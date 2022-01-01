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

#include <csignal>
#include <stdexcept>
#include "ThreadCpuTimer.h"

using namespace ascee;
using namespace ascee::runtime;

ThreadCpuTimer::ThreadCpuTimer() {
    timer = nullptr;
    thread = pthread_self();

    // Creating the execution timer
    struct sigevent sev{};
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGALRM;
    sev.sigev_value.sival_ptr = &thread;
    if (timer_create(CLOCK_THREAD_CPUTIME_ID, &sev, &timer) != 0) {
        throw std::runtime_error("error in creating the cpu timer");
    }
}

ThreadCpuTimer::~ThreadCpuTimer() {
    timer_delete(timer);
    printf("timer deleted\n");
}

/// in the implementation of this function nsec = 0 should always stop the timer.
int64_t ThreadCpuTimer::setAlarm(int64_t nsec) {
    struct itimerspec its{};

    // Get the remaining time
    timer_gettime(timer, &its);
    int64_t remaining = its.it_value.tv_sec * 1000000000 + its.it_value.tv_nsec;

    // reset the timer
    its.it_value.tv_sec = nsec / 1000000000;
    its.it_value.tv_nsec = nsec % 1000000000;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    if (timer_settime(timer, 0, &its, nullptr) != 0) {
        throw std::runtime_error("error in starting the timer");
    }
    return remaining;
}

