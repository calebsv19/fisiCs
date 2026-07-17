#ifndef WORKSPACE_SANDBOX_STAGE_G_COMMON_H
#define WORKSPACE_SANDBOX_STAGE_G_COMMON_H

#include "workspace_sandbox/workspace_sandbox_runtime.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

static void ws_stageg_trace(const char *name, const char *detail, uint64_t value) {
    printf("TRACE|1|%s|detail=%s|value=%" PRIu64 "|result=1\n",
           name,
           detail ? detail : "ok",
           value);
}

static int32_t ws_stageg_quantize(float value) {
    double scaled = (double)value * 1000000.0;
    return (int32_t)(scaled >= 0.0 ? scaled + 0.5 : scaled - 0.5);
}

static uint64_t ws_stageg_hash_u32(uint64_t hash, uint32_t value) {
    unsigned int shift;
    for (shift = 0u; shift < 32u; shift += 8u) {
        hash ^= (uint64_t)((value >> shift) & 0xffu);
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static uint32_t ws_stageg_active_module_count(const WorkspaceSandboxModuleState *modules) {
    uint32_t count = 0u;
    uint32_t i;
    if (!modules) return 0u;
    for (i = 0u; i < modules->count; ++i) {
        if (modules->instances[i].active) count += 1u;
    }
    return count;
}

static uint64_t ws_stageg_semantic_hash(const WorkspaceSandboxRuntime *runtime) {
    uint64_t hash = UINT64_C(1469598103934665603);
    uint32_t i;
    const WorkspaceSandboxLayout *layout;
    const WorkspaceSandboxModuleState *modules;
    if (!runtime) return 0u;
    layout = &runtime->active_layout;
    modules = &runtime->active_modules;
    hash = ws_stageg_hash_u32(hash, layout->node_count);
    hash = ws_stageg_hash_u32(hash, layout->root_index);
    for (i = 0u; i < layout->node_count; ++i) {
        const CorePaneNode *node = &layout->nodes[i];
        hash = ws_stageg_hash_u32(hash, (uint32_t)node->type);
        hash = ws_stageg_hash_u32(hash, node->id);
        if (node->type == CORE_PANE_NODE_LEAF) continue;
        hash = ws_stageg_hash_u32(hash, (uint32_t)node->axis);
        hash = ws_stageg_hash_u32(hash, (uint32_t)ws_stageg_quantize(node->ratio_01));
        hash = ws_stageg_hash_u32(hash, node->child_a);
        hash = ws_stageg_hash_u32(hash, node->child_b);
        hash = ws_stageg_hash_u32(hash, (uint32_t)ws_stageg_quantize(node->constraints.min_size_a));
        hash = ws_stageg_hash_u32(hash, (uint32_t)ws_stageg_quantize(node->constraints.min_size_b));
    }
    hash = ws_stageg_hash_u32(hash, ws_stageg_active_module_count(modules));
    for (i = 0u; i < modules->count; ++i) {
        const WorkspaceSandboxModuleInstance *instance = &modules->instances[i];
        if (!instance->active) continue;
        hash = ws_stageg_hash_u32(hash, instance->instance_id);
        hash = ws_stageg_hash_u32(hash, instance->pane_id);
        hash = ws_stageg_hash_u32(hash, (uint32_t)instance->kind);
        hash = ws_stageg_hash_u32(hash, (uint32_t)instance->active);
    }
    return hash;
}

static int ws_stageg_semantic_equal(const WorkspaceSandboxRuntime *a,
                                    const WorkspaceSandboxRuntime *b) {
    uint32_t i;
    if (!a || !b) return 0;
    if (a->active_layout.node_count != b->active_layout.node_count ||
        a->active_layout.root_index != b->active_layout.root_index ||
        ws_stageg_active_module_count(&a->active_modules) !=
            ws_stageg_active_module_count(&b->active_modules)) return 0;
    for (i = 0u; i < a->active_layout.node_count; ++i) {
        const CorePaneNode *na = &a->active_layout.nodes[i];
        const CorePaneNode *nb = &b->active_layout.nodes[i];
        if (na->type != nb->type || na->id != nb->id) return 0;
        if (na->type == CORE_PANE_NODE_LEAF) continue;
        if (na->axis != nb->axis ||
            ws_stageg_quantize(na->ratio_01) != ws_stageg_quantize(nb->ratio_01) ||
            na->child_a != nb->child_a || na->child_b != nb->child_b ||
            ws_stageg_quantize(na->constraints.min_size_a) != ws_stageg_quantize(nb->constraints.min_size_a) ||
            ws_stageg_quantize(na->constraints.min_size_b) != ws_stageg_quantize(nb->constraints.min_size_b)) return 0;
    }
    for (i = 0u; i < a->active_modules.count; ++i) {
        const WorkspaceSandboxModuleInstance *ma = &a->active_modules.instances[i];
        uint32_t j;
        int found = 0;
        if (!ma->active) continue;
        for (j = 0u; j < b->active_modules.count; ++j) {
            const WorkspaceSandboxModuleInstance *mb = &b->active_modules.instances[j];
            if (mb->active && mb->pane_id == ma->pane_id) {
                if (ma->instance_id != mb->instance_id || ma->kind != mb->kind) return 0;
                found = 1;
                break;
            }
        }
        if (!found) return 0;
    }
    return 1;
}

static int ws_stageg_write_canonical(const WorkspaceSandboxRuntime *runtime,
                                     const char *path,
                                     const char *label) {
    FILE *fp;
    uint32_t i;
    if (!runtime || !path || !label) return 0;
    fp = fopen(path, "wb");
    if (!fp) return 0;
    fprintf(fp,
            "label=%s\nmode=%d\nactive_revision=%" PRIu64 "\nnode_count=%u\nroot=%u\nactive_module_count=%u\nhash=%016" PRIx64 "\n",
            label,
            (int)runtime->layout_state.mode,
            runtime->layout_state.active_revision,
            runtime->active_layout.node_count,
            runtime->active_layout.root_index,
            ws_stageg_active_module_count(&runtime->active_modules),
            ws_stageg_semantic_hash(runtime));
    for (i = 0u; i < runtime->active_layout.node_count; ++i) {
        const CorePaneNode *node = &runtime->active_layout.nodes[i];
        if (node->type == CORE_PANE_NODE_LEAF) {
            fprintf(fp, "node=%u,type=%d,id=%u\n", i, (int)node->type, node->id);
            continue;
        }
        fprintf(fp,
                "node=%u,type=%d,id=%u,axis=%d,ratio=%d,a=%u,b=%u,min_a=%d,min_b=%d\n",
                i,
                (int)node->type,
                node->id,
                (int)node->axis,
                ws_stageg_quantize(node->ratio_01),
                node->child_a,
                node->child_b,
                ws_stageg_quantize(node->constraints.min_size_a),
                ws_stageg_quantize(node->constraints.min_size_b));
    }
    for (i = 0u; i < runtime->active_modules.count; ++i) {
        const WorkspaceSandboxModuleInstance *instance = &runtime->active_modules.instances[i];
        if (!instance->active) continue;
        fprintf(fp,
                "module=%u,instance=%u,pane=%u,kind=%d,active=%u\n",
                i,
                instance->instance_id,
                instance->pane_id,
                (int)instance->kind,
                (unsigned int)instance->active);
    }
    return fclose(fp) == 0;
}

#endif
