#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "orca/pv_profiler.h"

#ifdef __PV_PROFILING_MODE__

#define PV_MAX_PROFILE_ENTRIES (50)

typedef struct pv_profiler_data {
    const char* section_name; // Name of the function or section being profiled
    double total_time;
    int32_t call_count;
    double last_start_time;
} pv_profiler_data_t;

static pv_profiler_data_t profiles[PV_MAX_PROFILE_ENTRIES];
static int32_t profile_entry_count = 0;

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double) tv.tv_sec + (double) tv.tv_usec * 1e-6;
}

void pv_profiler_start(const char* name) {
    int32_t idx = -1;
    for (int32_t i = 0; i < profile_entry_count; i++) {
        if (strcmp(profiles[i].section_name, name) == 0) {
            idx = i;
            break;
        }
    }

    if (idx == -1 && profile_entry_count < PV_MAX_PROFILE_ENTRIES) {
        idx = profile_entry_count++;
        profiles[idx].section_name = name;
        profiles[idx].total_time = 0;
        profiles[idx].call_count = 0;
    }

    if (idx != -1) {
        profiles[idx].last_start_time = get_time();
    }
}

void pv_profiler_stop(const char* name) {
    for (int32_t i = 0; i < profile_entry_count; i++) {
        if (strcmp(profiles[i].section_name, name) == 0) {
            profiles[i].total_time += (get_time() - profiles[i].last_start_time);
            profiles[i].call_count++;
            break;
        }
    }
}

void pv_profiler_reset(void) {
    profile_entry_count = 0;
    for (int32_t i = 0; i < PV_MAX_PROFILE_ENTRIES; i++) {
        profiles[i].section_name = NULL;
        profiles[i].total_time = 0;
        profiles[i].call_count = 0;
        profiles[i].last_start_time = 0;
    }
}

void pv_profiler_print_data(void) {
    for (int32_t i = 0; i < profile_entry_count; i++) {
        for (int32_t j = i + 1; j < profile_entry_count; j++) {
            if (profiles[i].total_time < profiles[j].total_time) {
                pv_profiler_data_t tmp = profiles[i];
                profiles[i] = profiles[j];
                profiles[j] = tmp;
            }
        }
    }

    printf("Profiling Data:\n");
    for (int32_t i = 0; i < profile_entry_count; i++) {
        printf("%s: %.4f s (called %u times)\n",
               profiles[i].section_name,
               profiles[i].total_time,
               profiles[i].call_count);
    }
}

#else

void pv_profiler_start(const char* name) {
    (void) name;
    // Empty
}

void pv_profiler_stop(const char* name) {
    (void) name;
    // Empty
}

void pv_profiler_reset(void) {
    // Empty
}

void pv_profiler_print_data(void) {
    // Empty
}

#endif
