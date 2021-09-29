
#include "../../../include/argc/types.h"
#include "session.h"

using namespace ascee;

extern "C"
int64 loadInt64(int32 offset) {
    return static_cast<SessionInfo*>(session)->heapModifier->loadInt64(offset);
}

