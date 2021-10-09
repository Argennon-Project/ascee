#ifndef ASCEE_ARGC_FUNCTIONS_H
#define ASCEE_ARGC_FUNCTIONS_H

#include "types.h"

#ifdef __cplusplus
namespace ascee::argcrt {

extern "C" {
#endif

int64 loadInt64(int32 offset);
int invoke_dispatcher(byte forwarded_gas, std_id_t app_id, string_t request);
void invoke_deferred(byte forwarded_gas, std_id_t app_id, string_t request);
void enter_area();
void exit_area();
void append_str(string_buffer*, string_t);
void append_int64(string_buffer*, int64);
string_buffer* response_buffer();
void clear_buffer(string_buffer* buf);
string_t buf_to_string(const string_buffer* buf);

#ifdef __cplusplus
}

} // namespace argcrt
#endif
#endif // ASCEE_ARGC_FUNCTIONS_H