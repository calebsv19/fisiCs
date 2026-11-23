// SPDX-License-Identifier: Apache-2.0

#include "main_internal.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

#ifndef FISICS_MEMCHECK_RUNTIME_RELPATH
#define FISICS_MEMCHECK_RUNTIME_RELPATH "build/unsanitized/libfisics_memcheck_runtime.a"
#endif

static char* canonical_executable_path(void) {
#if defined(__APPLE__)
    uint32_t size = 1u;
    char placeholder = '\0';
    if (_NSGetExecutablePath(&placeholder, &size) != 0 && size > 1u) {
        char* path = (char*)malloc((size_t)size);
        if (!path) return NULL;
        if (_NSGetExecutablePath(path, &size) == 0) {
            char* canonical = realpath(path, NULL);
            free(path);
            if (canonical) return canonical;
        } else {
            free(path);
        }
    }
    return NULL;
#elif defined(__linux__)
    size_t capacity = 256u;
    while (capacity <= 1024u * 1024u) {
        char* path = (char*)malloc(capacity);
        if (!path) return NULL;
        ssize_t length = readlink("/proc/self/exe", path, capacity - 1u);
        if (length >= 0 && (size_t)length < capacity - 1u) {
            path[length] = '\0';
            char* canonical = realpath(path, NULL);
            free(path);
            if (canonical) return canonical;
            break;
        }
        free(path);
        if (length < 0) break;
        capacity *= 2u;
    }
    return NULL;
#else
    return NULL;
#endif
}

static char* canonical_regular_archive(const char* path,
                                       const char* description) {
    char* canonical = realpath(path, NULL);
    if (!canonical) {
        fprintf(stderr,
                "Error: %s is unavailable: %s\n",
                description,
                path ? path : "(null)");
        return NULL;
    }
    struct stat status;
    if (stat(canonical, &status) != 0 || !S_ISREG(status.st_mode)) {
        fprintf(stderr,
                "Error: %s is not a regular file: %s\n",
                description,
                canonical);
        free(canonical);
        return NULL;
    }
    return canonical;
}

char* main_resolve_memory_check_runtime_archive(void) {
    const char* override = getenv("FISICS_MEMCHECK_RUNTIME_LIB");
    if (override && override[0]) {
        if (override[0] != '/') {
            fprintf(stderr,
                    "Error: FISICS_MEMCHECK_RUNTIME_LIB must be an absolute path\n");
            return NULL;
        }
        return canonical_regular_archive(override,
                                         "memory-check runtime override");
    }

    char* executable = canonical_executable_path();
    if (!executable) {
        fprintf(stderr,
                "Error: cannot locate the compiler executable for the memory-check runtime; set FISICS_MEMCHECK_RUNTIME_LIB to a trusted absolute path\n");
        return NULL;
    }
    char* slash = strrchr(executable, '/');
    if (!slash) {
        fprintf(stderr,
                "Error: cannot derive the compiler installation root for the memory-check runtime\n");
        free(executable);
        return NULL;
    }
    *slash = '\0';

    size_t candidateLen = strlen(executable) +
                          strlen(FISICS_MEMCHECK_RUNTIME_RELPATH) + 2u;
    char* candidate = (char*)malloc(candidateLen);
    if (!candidate) {
        fprintf(stderr, "OOM: memory-check runtime path\n");
        free(executable);
        return NULL;
    }
    snprintf(candidate,
             candidateLen,
             "%s/%s",
             executable,
             FISICS_MEMCHECK_RUNTIME_RELPATH);
    free(executable);

    char* canonical = canonical_regular_archive(
        candidate,
        "trusted compiler-relative memory-check runtime archive");
    free(candidate);
    return canonical;
}
