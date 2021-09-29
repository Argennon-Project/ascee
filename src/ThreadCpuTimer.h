
#ifndef ASCEE_THREADCPUTIMER_H
#define ASCEE_THREADCPUTIMER_H

#include <ctime>
#include <pthread.h>

namespace ascee {

class ThreadCpuTimer {
private:
    pthread_t thread;
    timer_t timer;
public:
    ThreadCpuTimer();

    virtual ~ThreadCpuTimer();

    int64_t setAlarm(int64_t nsec);
};

} // namespace ascee

#endif // ASCEE_THREADCPUTIMER_H
