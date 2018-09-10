// macros.h - common macros to speed implementation.

#ifndef ECS_MACROS_H
#define ECS_MACROS_H

#include <stdio.h>

#define ERR_NO_RET(cond, ...) if (!cond) { \
    fprintf(stderr, __VA_ARGS__); \
}

#define ERR_OOM(cond, msg) if (!cond) { \
    fprintf(stderr, "ERROR " msg ": Out of Memory.\n"); \
    return NULL; \
}

#define ERR_CONTINUE(cond, ...) if (!cond) { \
    fprintf(stderr, __VA_ARGS__); \
    continue; \
}

#define ERR_RET_NULL(cond, ...) if (!cond) { \
    fprintf(stderr, __VA_ARGS__); \
    return NULL; \
}

#define ERR_RET_ZERO(cond, ...) if (!cond) { \
    fprintf(stderr, __VA_ARGS__); \
    return 0; \
}

#define ERR_ABORT(cond, ...) if (!cond) {\
    fprintf(stderr, __VA_ARGS__); \
    abort(); \
}

#endif
