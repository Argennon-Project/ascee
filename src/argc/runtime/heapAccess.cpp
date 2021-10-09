
#include <Executor.h>
#include "argc/types.h"

using namespace ascee;
using namespace argc;

extern "C"
int64 loadInt64(int32 offset) {
    try {
        return Executor::getSession()->heapModifier->load<int64>(offset);
    } catch (const std::out_of_range&) {
        raise(SIGSEGV);
    }
}

