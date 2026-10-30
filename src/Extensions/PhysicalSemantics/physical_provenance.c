// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_provenance.h"
#include "Extensions/PhysicalSemantics/physical_semantics_internal.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
uint64_t physical_bytes_hash(const char *s, size_t n) {
  uint64_t h = UINT64_C(1469598103934665603);
  for (size_t i = 0; i < n; i++) {
    h ^= (unsigned char)s[i];
    h *= UINT64_C(1099511628211);
  }
  return h;
}
bool physical_file_receipt(const char *path, FisicsPhysicalInputReceipt *out) {
  struct stat st;
  if (!path || stat(path, &st) || !S_ISREG(st.st_mode) || st.st_size < 0 ||
      (uint64_t)st.st_size > FISICS_PHYSICAL_INPUT_MAX_BYTES)
    return false;
  FILE *f = fopen(path, "rb");
  if (!f)
    return false;
  size_t n = 0;
  uint64_t h = UINT64_C(1469598103934665603);
  unsigned char b[8192];
  size_t k;
  while ((k = fread(b, 1, sizeof(b), f))) {
    if (n > FISICS_PHYSICAL_INPUT_MAX_BYTES - k) {
      fclose(f);
      return false;
    }
    n += k;
    for (size_t i = 0; i < k; i++) {
      h ^= b[i];
      h *= UINT64_C(1099511628211);
    }
  }
  bool ok = !ferror(f);
  if (fclose(f))
    ok = false;
  if (ok)
    *out = (FisicsPhysicalInputReceipt){h, n};
  return ok;
}
void physical_sources_free(FisicsPhysicalState *s) {
  for (size_t i = 0; i < s->source_count; i++)
    free(s->sources[i].path);
  free(s->sources);
  s->sources = NULL;
  s->source_count = 0;
  for (size_t i = 0; i < s->effective_include_count; i++)
    free(s->effective_include_paths[i]);
  free(s->effective_include_paths);
  s->effective_include_paths = NULL;
  s->effective_include_count = 0;
}
static int order(const void *a, const void *b) {
  return strcmp(((const PhysicalSourceReceipt *)a)->path,
                ((const PhysicalSourceReceipt *)b)->path);
}
bool physical_capture_sources(CompilerContext *c,
                              const IncludeResolver *resolver) {
  FisicsPhysicalState *s = physical_state(c);
  if (!s || !s->capture_sources)
    return true;
  s->closure_complete = false;
  if (!resolver || !resolver->count || resolver->count > 4096 ||
      getenv("FISICS_TEST_FAIL_PHYSICAL_CLOSURE"))
    return false;
  if (resolver->includePathCount > 4096)
    return false;
  s->effective_include_paths =
      calloc(resolver->includePathCount ? resolver->includePathCount : 1,
             sizeof(char *));
  if (!s->effective_include_paths)
    return false;
  for (size_t i = 0; i < resolver->includePathCount; i++) {
    s->effective_include_paths[i] = strdup(resolver->includePaths[i]);
    if (!s->effective_include_paths[i])
      return false;
    ++s->effective_include_count;
  }
  s->sources = calloc(resolver->count, sizeof(*s->sources));
  if (!s->sources)
    return false;
  size_t total = 0;
  for (size_t i = 0; i < resolver->count; i++) {
    const IncludeFile *f = &resolver->files[i];
    char *path = f->path ? realpath(f->path, NULL) : NULL;
    if (!path || !f->contents) {
      free(path);
      return false;
    }
    size_t bytes = strlen(f->contents);
    if (bytes > FISICS_PHYSICAL_INPUT_MAX_BYTES ||
        total > FISICS_PHYSICAL_TOTAL_MAX_BYTES - bytes) {
      free(path);
      return false;
    }
    total += bytes;
    FisicsPhysicalInputReceipt receipt = {
        physical_bytes_hash(f->contents, bytes), bytes};
    /* Hash the exact resolver buffers, including empty/guard-only headers.
       A later file read validates freshness, never substitutes analyzed bytes.
     */
    bool duplicate = false;
    for (size_t j = 0; j < s->source_count; j++)
      if (!strcmp(s->sources[j].path, path)) {
        if (s->sources[j].receipt.hash != receipt.hash ||
            s->sources[j].receipt.bytes != receipt.bytes) {
          free(path);
          return false;
        }
        duplicate = true;
        break;
      }
    if (duplicate) {
      free(path);
      continue;
    }
    s->sources[s->source_count++] = (PhysicalSourceReceipt){path, receipt};
  }
  qsort(s->sources, s->source_count, sizeof(*s->sources), order);
  s->closure_complete = true;
  return true;
}
