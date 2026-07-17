#pragma once

#include "render/runtime_native_3d_render_request_snapshot.h"

#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint64_t rt_stageg_hash_bytes(uint64_t hash, const void *data, size_t size) {
    const unsigned char *bytes = (const unsigned char *)data;
    size_t i;
    for (i = 0u; i < size; ++i) {
        hash ^= (uint64_t)bytes[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static uint64_t rt_stageg_hash_text(uint64_t hash, const char *text) {
    return rt_stageg_hash_bytes(hash, text ? text : "", text ? strlen(text) : 0u);
}

static void rt_stageg_trace(const char *name, const char *detail, uint64_t digest) {
    printf("TRACE|1|%s|detail=%s|digest=%016llx|result=1\n",
           name, detail ? detail : "ok", (unsigned long long)digest);
}

static uint64_t rt_stageg_snapshot_digest(
    const RuntimeNative3DRenderRequestSnapshot *snapshot) {
    uint64_t hash = UINT64_C(1469598103934665603);
    char line[512];
    if (!snapshot) return 0u;
    snprintf(line, sizeof(line),
             "valid=%d|generation=%d:%llu|output=%dx%d|render=%dx%d|host=%dx%d|"
             "frame=%d:%d:%d|tile=%d|integrator=%d|sampling=%d:%u:%u:%u|"
             "budget=%d:%d:%d:%d|prepared=%d:%d:%dx%d:%llu:%llu|"
             "material=%d:%llu:%llu|light=%d:%llu:%llu|accel=%d:%d:%llu:%llu|"
             "volume=%d:%d:%d|water=%d:%d:%d:%d:%llu:%d|ledger=%d|"
             "dest=%d:%s:%d:%s:%d:%s|cancel=%d:%llu",
             snapshot->valid ? 1 : 0, snapshot->generationBound ? 1 : 0,
             (unsigned long long)snapshot->generation,
             snapshot->outputWidth, snapshot->outputHeight,
             snapshot->renderWidth, snapshot->renderHeight,
             snapshot->hostWidth, snapshot->hostHeight,
             snapshot->frameIndex, snapshot->frameCount, snapshot->temporalFrames,
             snapshot->tileSize, (int)snapshot->integratorId,
             snapshot->samplingBound ? 1 : 0, snapshot->sampling.sampleSequence,
             (unsigned)snapshot->sampling.temporalSubpassIndex,
             (unsigned)snapshot->sampling.temporalSubpassCount,
             snapshot->resourceBudgetBound ? 1 : 0,
             snapshot->resourceBudget.cpuPercent,
             snapshot->resourceBudget.maxWorkerThreads,
             snapshot->resourceBudget.reserveCpuCount,
             snapshot->preparedFrameBound ? 1 : 0,
             snapshot->preparedFrameValid ? 1 : 0,
             snapshot->preparedFrameWidth, snapshot->preparedFrameHeight,
             (unsigned long long)snapshot->preparedPrimitiveCount,
             (unsigned long long)snapshot->preparedTriangleCount,
             snapshot->materialSnapshotBound ? 1 : 0,
             (unsigned long long)snapshot->materialCount,
             (unsigned long long)snapshot->materialObjectBindingCount,
             snapshot->lightSnapshotBound ? 1 : 0,
             (unsigned long long)snapshot->enabledLightCount,
             (unsigned long long)snapshot->materialEmitterLightCount,
             snapshot->sceneAccelerationBound ? 1 : 0, (int)snapshot->traceRoute,
             (unsigned long long)snapshot->tlasInstanceCount,
             (unsigned long long)snapshot->tlasNodeCount,
             snapshot->volumeEnabled ? 1 : 0, snapshot->volumeAttached ? 1 : 0,
             snapshot->volumeFrameSelectionDynamic ? 1 : 0,
             snapshot->waterSurfaceSourceFound ? 1 : 0,
             snapshot->waterSurfaceLoaded ? 1 : 0,
             snapshot->waterSurfaceMeshAttached ? 1 : 0,
             snapshot->waterSurfaceFrameSelectionDynamic ? 1 : 0,
             (unsigned long long)snapshot->waterSurfaceSampleCount,
             snapshot->waterSurfaceTriangleCount,
             snapshot->frameDataflowLedgerEnabled ? 1 : 0,
             snapshot->outputRootBound ? 1 : 0, snapshot->outputRoot,
             snapshot->summaryDestinationBound ? 1 : 0, snapshot->summaryPath,
             snapshot->progressDestinationBound ? 1 : 0, snapshot->progressPath,
             snapshot->cancelTokenBound ? 1 : 0,
             (unsigned long long)snapshot->cancelGeneration);
    return rt_stageg_hash_text(hash, line);
}

static int rt_stageg_build_snapshot(RuntimeNative3DRenderRequestSnapshot *snapshot) {
    static atomic_bool cancel_requested = ATOMIC_VAR_INIT(false);
    static const RuntimeNative3DSamplingContext sampling = {424242u, 2u, 5u};
    static const RuntimeNative3DResourceBudget budget = {75, 6, 2};
    RuntimeNative3DTileSchedulerCancelToken token = {&cancel_requested, 77u};
    RuntimeNative3DRenderRequestSnapshotDesc desc;
    memset(&desc, 0, sizeof(desc));
    desc.generationBound = true;
    desc.generation = 77u;
    desc.outputWidth = 512;
    desc.outputHeight = 288;
    desc.renderWidth = 1024;
    desc.renderHeight = 576;
    desc.hostWidth = 512;
    desc.hostHeight = 288;
    desc.frameIndex = 3;
    desc.frameCount = 12;
    desc.temporalFrames = 5;
    desc.tileSize = 32;
    desc.integratorId = RAY_TRACING_3D_INTEGRATOR_DIRECT_LIGHT;
    desc.sampling = &sampling;
    desc.resourceBudget = &budget;
    desc.preparedFrameBound = true;
    desc.preparedFrameValid = true;
    desc.preparedFrameWidth = 1024;
    desc.preparedFrameHeight = 576;
    desc.preparedPrimitiveCount = 19u;
    desc.preparedTriangleCount = 48000u;
    desc.materialSnapshotBound = true;
    desc.materialCount = 11u;
    desc.materialObjectBindingCount = 19u;
    desc.lightSnapshotBound = true;
    desc.enabledLightCount = 4u;
    desc.materialEmitterLightCount = 2u;
    desc.sceneAccelerationBound = true;
    desc.traceRoute = RUNTIME_RAY_3D_TRACE_ROUTE_TLAS_BLAS;
    desc.tlasInstanceCount = 19u;
    desc.tlasNodeCount = 37u;
    desc.traceContextCallbackBound = true;
    desc.volumeEnabled = true;
    desc.volumeAttached = true;
    desc.waterSurfaceSourceFound = true;
    desc.waterSurfaceLoaded = true;
    desc.waterSurfaceMeshAttached = true;
    desc.waterSurfaceSampleCount = 4096u;
    desc.waterSurfaceTriangleCount = 7938;
    desc.frameDataflowLedgerEnabled = true;
    desc.outputRoot = "ray_tracing/frames_temp";
    desc.summaryPath = "ray_tracing/frames_temp/render_summary.json";
    desc.progressPath = "ray_tracing/frames_temp/render_progress.json";
    desc.cancelToken = &token;
    return RuntimeNative3DRenderRequestSnapshot_Build(snapshot, &desc) ? 1 : 0;
}

static int rt_stageg_write_snapshot(const char *path,
                                    const RuntimeNative3DRenderRequestSnapshot *snapshot,
                                    uint64_t digest) {
    FILE *file = fopen(path, "wb");
    if (!file || !snapshot) return 0;
    fprintf(file,
            "schema=ray_tracing_stage_g_snapshot_v1\n"
            "generation=%llu\noutput=%dx%d\nrender=%dx%d\nhost=%dx%d\n"
            "frame=%d/%d\ntemporal=%d\ntile=%d\nprepared=%d:%dx%d:%llu:%llu\n"
            "materials=%llu:%llu\nlights=%llu:%llu\nacceleration=%d:%llu:%llu\n"
            "destinations=%s|%s|%s\ndigest=%016llx\n",
            (unsigned long long)snapshot->generation,
            snapshot->outputWidth, snapshot->outputHeight,
            snapshot->renderWidth, snapshot->renderHeight,
            snapshot->hostWidth, snapshot->hostHeight,
            snapshot->frameIndex, snapshot->frameCount, snapshot->temporalFrames,
            snapshot->tileSize, snapshot->preparedFrameValid ? 1 : 0,
            snapshot->preparedFrameWidth, snapshot->preparedFrameHeight,
            (unsigned long long)snapshot->preparedPrimitiveCount,
            (unsigned long long)snapshot->preparedTriangleCount,
            (unsigned long long)snapshot->materialCount,
            (unsigned long long)snapshot->materialObjectBindingCount,
            (unsigned long long)snapshot->enabledLightCount,
            (unsigned long long)snapshot->materialEmitterLightCount,
            (int)snapshot->traceRoute,
            (unsigned long long)snapshot->tlasInstanceCount,
            (unsigned long long)snapshot->tlasNodeCount,
            snapshot->outputRoot, snapshot->summaryPath, snapshot->progressPath,
            (unsigned long long)digest);
    return fclose(file) == 0;
}
