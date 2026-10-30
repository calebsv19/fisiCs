// SPDX-License-Identifier: Apache-2.0
#include "Compiler/physical_catalog_io.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
FisicsPhysicalLoadOptions fisics_physical_load_defaults(void) {
  return (FisicsPhysicalLoadOptions){FISICS_PHYSICAL_INPUT_MAX_BYTES,
                                     FISICS_PHYSICAL_TOTAL_MAX_BYTES,
                                     65536u,
                                     262144u,
                                     1024u,
                                     65536u,
                                     64u, 256u, 256u, 512u,
                                     SIZE_MAX};
}
bool fisics_physical_load_options_valid(FisicsPhysicalLoadOptions o) {
  FisicsPhysicalLoadOptions d = fisics_physical_load_defaults();
  return o.max_file_bytes && o.max_file_bytes <= d.max_file_bytes &&
         o.max_total_bytes && o.max_total_bytes <= d.max_total_bytes &&
         o.max_kinds && o.max_kinds <= d.max_kinds && o.max_contracts &&
         o.max_contracts <= d.max_contracts && o.max_catalogs &&
         o.max_catalogs <= d.max_catalogs && o.max_origins &&
         o.max_origins <= d.max_origins &&
         o.max_frame_contexts && o.max_frame_contexts <= d.max_frame_contexts &&
         o.max_reference_points && o.max_reference_points <= d.max_reference_points &&
         o.max_frames && o.max_frames <= d.max_frames &&
         o.max_frame_transforms && o.max_frame_transforms <= d.max_frame_transforms;
}
void physical_load_diag(FisicsSemanticProjectDiagnostic *d, const char *fmt,
                        ...) {
  if (!d || d->message[0])
    return;
  va_list args;
  va_start(args, fmt);
  vsnprintf(d->message, sizeof(d->message), fmt, args);
  va_end(args);
}
static bool allocation(FisicsPhysicalLoadContext *l) {
  if (!l)
    return true;
  if (l->allocations++ >= l->options.fail_after) {
    l->failure = PHYSICAL_LOAD_ALLOCATION;
    return false;
  }
  return true;
}
void *physical_alloc(FisicsPhysicalLoadContext *l, size_t n) {
  if (!allocation(l))
    return NULL;
  void *p = malloc(n ? n : 1);
  if (!p && l)
    l->failure = PHYSICAL_LOAD_ALLOCATION;
  return p;
}
void *physical_calloc(FisicsPhysicalLoadContext *l, size_t n, size_t w) {
  if (w && n > SIZE_MAX / w) {
    if (l)
      l->failure = PHYSICAL_LOAD_LIMIT;
    return NULL;
  }
  void *p = physical_alloc(l, n * w);
  if (p)
    memset(p, 0, n * w);
  return p;
}
void *physical_realloc(FisicsPhysicalLoadContext *l, void *old, size_t n) {
  if (!allocation(l))
    return NULL;
  void *p = realloc(old, n ? n : 1);
  if (!p && l)
    l->failure = PHYSICAL_LOAD_ALLOCATION;
  return p;
}
char *physical_dup_text(FisicsPhysicalLoadContext *l, const char *s) {
  if (!s)
    return NULL;
  size_t n = strlen(s) + 1;
  char *p = physical_alloc(l, n);
  if (p)
    memcpy(p, s, n);
  return p;
}
bool physical_exact_fields(const FisicsJsonValue *o, const char *const *fields,
                           size_t n) {
  if (!o || o->type != FISICS_JSON_OBJECT || o->as.object.count != n)
    return false;
  for (size_t i = 0; i < n; i++)
    if (!fisics_json_object_get(o, fields[i]))
      return false;
  return true;
}
bool fisics_physical_read_json(const char *path, size_t maximum,
                               FisicsJsonValue **out,
                               FisicsPhysicalInputReceipt *receipt,
                               FisicsPhysicalLoadContext *l,
                               FisicsSemanticProjectDiagnostic *d) {
  struct stat st;
  if (!out)
    return false;
  if (!path || stat(path, &st) || !S_ISREG(st.st_mode)) {
    if (l)
      l->failure = PHYSICAL_LOAD_IO;
    physical_load_diag(d, "physical project input is not a regular file: %s",
                       path ? path : "<null>");
    return false;
  }
  if (l && l->options.max_file_bytes < maximum)
    maximum = l->options.max_file_bytes;
  if (st.st_size < 0 || (uint64_t)st.st_size > maximum) {
    if (l)
      l->failure = PHYSICAL_LOAD_LIMIT;
    physical_load_diag(d, "physical project input exceeds %zu bytes: %s",
                       maximum, path);
    return false;
  }
  size_t n = (size_t)st.st_size;
  if (l && (n > l->options.max_total_bytes ||
            l->input_bytes > l->options.max_total_bytes - n)) {
    l->failure = PHYSICAL_LOAD_LIMIT;
    physical_load_diag(
        d, "physical project inputs exceed aggregate %zu byte limit",
        l->options.max_total_bytes);
    return false;
  }
  FILE *f = fopen(path, "rb");
  if (!f) {
    if (l)
      l->failure = PHYSICAL_LOAD_IO;
    physical_load_diag(d, "failed to read physical project input: %s", path);
    return false;
  }
  char *bytes = physical_alloc(l, n + 1);
  bool ok =
      bytes && fread(bytes, 1, n, f) == n && fgetc(f) == EOF && !ferror(f);
  if (fclose(f))
    ok = false;
  if (!ok) {
    free(bytes);
    if (l && l->failure == PHYSICAL_LOAD_OK)
      l->failure = PHYSICAL_LOAD_IO;
    physical_load_diag(d, "failed to read complete physical project input: %s",
                       path);
    return false;
  }
  bytes[n] = 0;
  FisicsJsonDiagnostic jd = {0};
  FisicsJsonValue *root = NULL;
  ok = fisics_json_parse(bytes, n, &root, &jd);
  if (!ok) {
    physical_load_diag(d, "invalid physical project JSON %s: %s", path,
                       jd.message);
    free(bytes);
    return false;
  }
  /* Same repository FNV-1a64 convention as existing source/registry receipts;
   * accidental-drift identity only, not cryptographic authentication. */
  uint64_t hash = UINT64_C(1469598103934665603);
  for (size_t i = 0; i < n; i++) {
    hash ^= (unsigned char)bytes[i];
    hash *= UINT64_C(1099511628211);
  }
  free(bytes);
  if (l)
    l->input_bytes += n;
  if (receipt)
    *receipt = (FisicsPhysicalInputReceipt){hash, n};
  *out = root;
  return true;
}
