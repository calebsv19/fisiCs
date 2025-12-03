#ifndef UTILS_H
#define UTILS_H

// Toggles printing the full source buffer after loading
#define DEBUG_PRINT_SOURCE 0

// Resolved from makefile's INCLUDE_PATHS; provided by the build.
#ifndef DEFAULT_INCLUDE_PATHS
#define DEFAULT_INCLUDE_PATHS "include"
#endif

// Reads a file's content into a null-terminated string
char* readFile(const char* filename);

#endif // UTILS_H
