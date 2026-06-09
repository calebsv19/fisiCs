#ifndef FISICS_MEMCHECK_RUNTIME_H
#define FISICS_MEMCHECK_RUNTIME_H

#include <stddef.h>

void* __fisics_memcheck_malloc(size_t size);
void* __fisics_memcheck_calloc(size_t count, size_t size);
void* __fisics_memcheck_realloc(void* ptr, size_t size);
void __fisics_memcheck_free(void* ptr);

void* __fisics_memcheck_malloc_site(size_t size, const char* file, int line);
void* __fisics_memcheck_calloc_site(size_t count, size_t size, const char* file, int line);
void* __fisics_memcheck_realloc_site(void* ptr, size_t size, const char* file, int line);
void __fisics_memcheck_free_site(void* ptr, const char* file, int line);

void __fisics_memcheck_report(void);
void __fisics_memcheck_reset(void);

#endif
