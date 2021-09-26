
#include "../../../include/argc/types.h"
#include "session.h"

extern "C"
int64 loadInt64(void* session, int32 offset) {
    return static_cast<SessionInfo*>(session)->heapModifier->loadInt64(offset);
}

