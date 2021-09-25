#ifndef ASCEE_ARGC_RUNTIME_H_
#define ASCEE_ARGC_RUNTIME_H_

int64 loadInt64(void* session, int32 offset);

void append_str(string_buffer*, string_t);

void append_int64(string_buffer*, int64);

#endif // ASCEE_ARGC_RUNTIME_H_