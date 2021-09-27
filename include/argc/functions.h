#ifndef ASCEE_ARGC_FUNCTIONS_H
#define ASCEE_ARGC_FUNCTIONS_H

int64 loadInt64(void* session, int32 offset);

//int invoke_dispatcher(void* session, byte forwarded_gas, std_id_t app_id, string_t request);

void append_str(string_buffer*, string_t);

void append_int64(string_buffer*, int64);

#endif // ASCEE_ARGC_FUNCTIONS_H