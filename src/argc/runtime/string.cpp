
#include <csignal>
#include <cstring>

#include <string>

#include "../../../include/argc/types.h"

using std::string;

extern "C"
void append_str(StringBuffer* buf, String str) {
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
void append_int64(StringBuffer* buf, int64 i) {
    string str = std::to_string(i);
    append_str(buf, String{str.c_str(), static_cast<int>(str.size() + 1)});
}

extern "C" inline
String buf_to_string(const StringBuffer* buf) {
    return String{
            .content = buf->buffer,
            .length = buf->end
    };
}
