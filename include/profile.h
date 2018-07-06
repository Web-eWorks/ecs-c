// profile.h

#ifndef ECS_PROFILE_H
#define ECS_PROFILE_H

/*
    A rudimentary set of macros for performance profiling.
*/

#include <time.h>
#include <stdio.h>

#define TIME_FACTOR_MS ((float)CLOCKS_PER_SEC / 1000)
#define TIME_FACTOR_US ((float)CLOCKS_PER_SEC / 1000000)

#define clock_to_s(a, b) ((float)(a) / b)

// disable performance counters when not debugging.
#ifdef NO_PROFILING

#define PERF_START()
#define PERF_UPDATE()
#define PERF_PRINT_MS(n)
#define PERF_PRINT_US(n)

#else

#define PERF_START() clock_t PERF_UPDATE()
#define PERF_UPDATE() _perf_ctr = clock()
#define PERF_PRINT_MS(n) printf(n " took %.2fms to complete.\n", clock_to_s(clock() - _perf_ctr, TIME_FACTOR_MS))
#define PERF_PRINT_US(n) printf(n " took %.2fus to complete.\n", clock_to_s(clock() - _perf_ctr, TIME_FACTOR_US))

#endif

#endif /* end of include guard: ECS_PROFILE_H */
