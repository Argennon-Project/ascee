
#include <csignal>
#include <stdexcept>
#include "ThreadCpuTimer.h"

using namespace ascee;

ThreadCpuTimer::ThreadCpuTimer() {
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

