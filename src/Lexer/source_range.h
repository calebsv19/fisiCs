// SPDX-License-Identifier: Apache-2.0

#ifndef SOURCE_RANGE_H
#define SOURCE_RANGE_H

typedef struct {
    const char* file;
    int line;
    int column;
} SourceLocation;

typedef struct {
    SourceLocation start;
    SourceLocation end;
} SourceRange;

#endif // SOURCE_RANGE_H
