// SPDX-License-Identifier: Apache-2.0
#include "Compiler/physical_receipt.h"
#include "Extensions/PhysicalSemantics/physical_frame_semantics.h"
#include <string.h>
void physical_frame_fact_write(FILE *out, CompilerContext *c, PhysicalFact f) {
  const FisicsPhysicalPolicy *p = &physical_state(c)->policy;
  PhysicalDeclaredContext v = physical_frame_context(c, f);
  const char *keys[] = {"frame",           "component_axis",
                        "applied_at",      "displacement_from",
                        "displacement_to", "moment_reference",
                        "point_subject"};
  uint32_t refs[] = {v.applied_at, v.displacement_from, v.displacement_to,
                     v.moment_reference, v.point_subject};
  fputc('{', out);
  for (unsigned i = 0; i < 7; i++) {
    if (i)
      fputc(',', out);
    physical_json_string(out, keys[i]);
    fputc(':', out);
    physical_json_string(
        out, i == 0   ? (v.frame ? p->frames[v.frame - 1].id : NULL)
             : i == 1 ? (v.axis == 1   ? "axis.x"
                         : v.axis == 2 ? "axis.y"
                         : v.axis == 3 ? "axis.z"
                                       : NULL)
                      : (refs[i - 2] ? p->reference_points[refs[i - 2] - 1].id
                                     : NULL));
  }
  bool geometric = physical_frame_geometric(c, f),
       known = physical_known(f) || f.state == PHYSICAL_CONTROL ||
               (v.flags & PHYSICAL_FRAME_INVARIANT),
       bad = f.state == PHYSICAL_ERROR;
  bool ref = v.applied_at || v.displacement_from || v.moment_reference ||
             v.point_subject;
  const char *identity = bad                   ? "contradictory"
                         : v.frame             ? "declared"
                         : geometric || !known ? "unknown"
                                               : "not_applicable";
  const char *compat = bad                                ? "contradictory"
                       : v.flags & PHYSICAL_FRAME_CHECKED ? "checked"
                       : geometric || !known              ? "unknown"
                                                          : "not_applicable";
  const char *premise = geometric ? (v.frame ? "declared" : "unassessed")
                        : known   ? "not_applicable"
                                  : "unassessed";
  fprintf(
      out,
      ",\"facets\":{\"frame_identity\":\"%s\",\"compatibility\":\"%s\","
      "\"geometry\":\"%s\",\"observer_state\":\"%s\",\"references\":\"%s\"}}",
      identity, compat, premise, premise,
      bad     ? "contradictory"
      : ref   ? (v.flags & PHYSICAL_FRAME_CHECKED ? "checked_compatible"
                                                  : "declared")
      : known ? "not_applicable"
              : "unknown");
}
