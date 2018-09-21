// macros.h - common macros to speed implementation.

#ifndef ECS_MACROS_H
#define ECS_MACROS_H

#include <stdio.h>

#define ERR(cond, action, ...) if (!(cond)) { \
    fprintf(stderr, __VA_ARGS__); \
    action; \
}

#define ERR_NO_RET(cond, ...) ERR(cond, , __VA_ARGS__)

#define ERR_OOM(cond, msg) ERR(cond, return NULL, "ERROR " msg ": Out of Memory.\n")

#define ERR_CONTINUE(cond, ...) ERR(cond, continue, __VA_ARGS__)

#define ERR_RET(cond, ...) ERR(cond, return, __VA_ARGS__)

#define ERR_RET_NULL(cond, ...) ERR(cond, return NULL, __VA_ARGS__)

#define ERR_RET_ZERO(cond, ...) ERR(cond, return 0, __VA_ARGS__)

#define ERR_ABORT(cond, ...) ERR(cond, abort(), __VA_ARGS__)

#endif
