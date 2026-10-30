// SPDX-License-Identifier: Apache-2.0
/* Test-only pointer ledger. No production allocator or global registry changes.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SLOTS (1u << 20)
static void *pointers[SLOTS];
static unsigned char states[SLOTS];
static size_t live;
static size_t lookup(const void *p) {
  size_t i = ((uintptr_t)p >> 4) & (SLOTS - 1);
  while (states[i]) {
    if (states[i] == 1 && pointers[i] == p)
      return i;
    i = (i + 1) & (SLOTS - 1);
  }
  return SLOTS;
}
static void insert(void *p) {
  if (!p)
    return;
  size_t i = ((uintptr_t)p >> 4) & (SLOTS - 1);
  while (states[i] == 1)
    i = (i + 1) & (SLOTS - 1);
  pointers[i] = p;
  states[i] = 1;
  live++;
}
char *physical_probe_strdup(const char *s) {
  char *p = strdup(s);
  insert(p);
  return p;
}
char *physical_probe_realpath(const char *s, char *out) {
  char *p = realpath(s, out);
  if (!out)
    insert(p);
  return p;
}
void *physical_probe_calloc(size_t n, size_t w) {
  void *p = calloc(n, w);
  insert(p);
  return p;
}
void *physical_probe_malloc(size_t n) {
  void *p = malloc(n);
  insert(p);
  return p;
}
void *physical_probe_realloc(void *old, size_t n) {
  size_t i = lookup(old);
  void *p = realloc(old, n);
  if (p) {
    if (i < SLOTS) {
      states[i] = 2;
      live--;
    }
    insert(p);
  }
  return p;
}
void physical_probe_free(void *p) {
  size_t i = lookup(p);
  if (i < SLOTS) {
    states[i] = 2;
    live--;
  }
  free(p);
}
__attribute__((destructor)) static void verify_owned_allocations(void) {
  if (live) {
    fprintf(stderr, "physical loader leaked %zu owned allocations\n", live);
    abort();
  }
}
