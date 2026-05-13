// SPDX-License-Identifier: Apache-2.0

#include "codegen_dispatch_diagnostics.h"

#include <stdio.h>

void codegenDispatchReportNullNode(void) {
    fprintf(stderr, "Error: NULL node in codegen\n");
}

void codegenDispatchReportUnhandledNodeType(int nodeType) {
    fprintf(stderr, "Error: Unhandled AST node type %d\n", nodeType);
}
