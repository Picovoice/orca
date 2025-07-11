#ifndef PV_PROFILER_H
#define PV_PROFILER_H

#include "core/pv_type.h"

void pv_profiler_start(const char* name);

void pv_profiler_stop(const char* name);

void pv_profiler_reset(void);

void pv_profiler_print_data(void);

#ifdef __PV_PROFILING_MODE__

#define PV_ORCA_PROFILER_START(n) pv_profiler_start(n)
#define PV_ORCA_PROFILER_STOP(n) pv_profiler_stop(n)
#define PV_ORCA_PROFILER_PRINT_DATA pv_profiler_print_data()

#else

#define PV_ORCA_PROFILER_START(n)
#define PV_ORCA_PROFILER_STOP(n)
#define PV_ORCA_PROFILER_PRINT_DATA

#endif

#endif
