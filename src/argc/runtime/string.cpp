
#include <csignal>
#include <cstring>

#include <string>
#include <argc/functions.h>

#include "argc/types.h"

using std::string;
using namespace ascee;

extern "C"
void argcrt::append_str(string_buffer* buf, string_t str) {
    if (buf->maxSize < buf->end + str.length) {
        raise(SIGSEGV);
    }
    strncpy(buf->buffer + buf->end, str.content, str.length);
    if (str.content[str.length - 1] == '\0')
        buf->end += str.length - 1;
    else
        buf->end += str.length;
}

extern "C"
void argcrt::append_int64(string_buffer* buf, int64 i) {
    string str = std::to_string(i);
    append_str(buf, string_t{str.c_str(), static_cast<int>(str.size() + 1)});
}

extern "C"
void argcrt::clear_buffer(string_buffer* buf) {
    buf->end = 0;
}

extern "C"
string_t buf_to_string(const string_buffer* buf) {
    return string_t{
            .content = buf->buffer,
            .length = buf->end
    };
}
