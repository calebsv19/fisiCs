// SPDX-License-Identifier: Apache-2.0

#ifndef PREPROCESSOR_EXTERNAL_H
#define PREPROCESSOR_EXTERNAL_H

#include <stdbool.h>
#include <stddef.h>

bool pp_run_external_preprocessor(const char* cmd,
                                  const char* args,
                                  const char* inputPath,
                                  const char* const* includePaths,
                                  size_t includePathCount,
                                  char** outBuffer,
                                  size_t* outLength);

#endif /* PREPROCESSOR_EXTERNAL_H */
