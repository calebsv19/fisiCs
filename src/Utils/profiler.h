// SPDX-License-Identifier: Apache-2.0

#ifndef FISICS_PROFILER_H
#define FISICS_PROFILER_H

#include <stdint.h>

typedef struct ProfilerScope {
    const char* name;
    uint64_t start_ns;
} ProfilerScope;

void profiler_init(void);
void profiler_shutdown(void);
int profiler_enabled(void);

ProfilerScope profiler_begin(const char* name);
void profiler_end(ProfilerScope scope);
void profiler_record_value(const char* name, uint64_t value);

#endif // FISICS_PROFILER_H
