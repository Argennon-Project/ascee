
#include <Executor.h>
#include "argc/types.h"

using namespace ascee;

extern "C"
int64 loadInt64(int32 offset) {
    return static_cast<SessionInfo*>(Executor::getSession())->heapModifier->loadInt64(offset);
}

