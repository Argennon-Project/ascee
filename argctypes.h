#ifndef INVOKER_ARGC_TYPES_H
#define INVOKER_ARGC_TYPES_H

#include <stdint.h>

typedef uint8_t byte;
typedef int32_t int32;
typedef int64_t int64;
typedef __int128_t int128;
typedef double float64;
typedef __float128 float128;
typedef uint32_t short_id_t;
typedef uint64_t std_id_t;
typedef __uint128_t full_id_t;

typedef int (* dispatcher_ptr_t)(void*, const char*, char*);

#define HTTP_OK 200
#define BAD_REQUEST 400
#define NOT_ACCEPTABLE 406
#define FATAL_ERROR 1
#define APP_NOT_FOUND 2
#define MALFORMED_APP 3
#define INTERNAL_ERROR 4
#define OUT_OF_TIME 5

#endif //INVOKER_ARGC_TYPES_H
