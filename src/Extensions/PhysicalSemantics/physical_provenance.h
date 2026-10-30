// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "Extensions/PhysicalSemantics/physical_semantics.h"
#include "Preprocessor/include_resolver.h"
typedef struct PhysicalSourceReceipt {
  char *path;
  FisicsPhysicalInputReceipt receipt;
} PhysicalSourceReceipt;
uint64_t physical_bytes_hash(const char *, size_t);
bool physical_file_receipt(const char *, FisicsPhysicalInputReceipt *);
bool physical_capture_sources(struct CompilerContext *,
                              const IncludeResolver *);
void physical_sources_free(FisicsPhysicalState *);
