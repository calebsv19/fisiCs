// SPDX-License-Identifier: Apache-2.0
#pragma once
/* Force-include only when building the five P5.2 modules for the allocation
 * ledger gate. Existing JSON/unit parser allocations are intentionally outside
 * this owned-model ledger. */
#include <stdlib.h>
#include <string.h>
void *physical_probe_malloc(size_t);
void *physical_probe_calloc(size_t, size_t);
void *physical_probe_realloc(void *, size_t);
void physical_probe_free(void *);
char *physical_probe_strdup(const char *);
char *physical_probe_realpath(const char *, char *);
#define malloc physical_probe_malloc
#define calloc physical_probe_calloc
#define realloc physical_probe_realloc
#define free physical_probe_free
#define strdup physical_probe_strdup
#define realpath physical_probe_realpath
