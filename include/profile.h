// profile.h

#ifndef ECS_PROFILE_H
#define ECS_PROFILE_H

/*
    A rudimentary set of macros for performance profiling.
*/

#include <time.h>
#include <stdio.h>

#define TIME_FACTOR_MS (1000.0)
#define TIME_FACTOR_US (1000000.0)

// disable performance counters when not debugging.
#ifndef NO_PROFILING

#define PERF_START() struct timespec _perf_ctr; PERF_UPDATE()
#define PERF_UPDATE() clock_gettime(CLOCK_MONOTONIC, &_perf_ctr)
#define PERF_PRINT_CUSTOM(f, ...) printf(..., profile_get(&_perf_ctr) / f)
#define PERF_PRINT_MS(n) printf(n " took %.2fms to complete.\n", profile_get(&_perf_ctr) * TIME_FACTOR_MS)
#define PERF_PRINT_US(n) printf(n " took %.2fus to complete.\n", profile_get(&_perf_ctr) * TIME_FACTOR_US)

#else

#define PERF_START()
#define PERF_UPDATE()
#define PERF_PRINT_CUSTOM(f, ...)
#define PERF_PRINT_MS(n)
#define PERF_PRINT_US(n)

#endif

static inline double profile_get(struct timespec *start)
{
    struct timespec end = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &end);

    return (double)(end.tv_sec - start->tv_sec) + (double)(end.tv_nsec - start->tv_nsec) / 1000000000.0;
}

#endif /* end of include guard: ECS_PROFILE_H */
