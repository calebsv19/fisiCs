#include "ray_tracing_stage_g_common.h"
#include "render/runtime_native_3d_async_render_bridge.h"

#include <stdlib.h>

static int expect_readiness(RuntimeNative3DRenderRequestSnapshot *snapshot,
                            RuntimeNative3DAsyncRenderReadiness expected) {
    RuntimeNative3DAsyncRenderAssessment assessment =
        RuntimeNative3DAsyncRender_AssessSnapshot(snapshot);
    return assessment.readiness == expected &&
           assessment.ready ==
               (expected == RUNTIME_NATIVE_3D_ASYNC_RENDER_READY_EXCLUSIVE_SINGLE_JOB) &&
           strcmp(RuntimeNative3DAsyncRenderReadiness_Name(expected), "unknown") != 0;
}

int main(void) {
    RuntimeNative3DRenderRequestSnapshot snapshot;
    RuntimeNative3DRenderRequestSnapshot base;
    RuntimeNative3DAsyncRenderAssessment assessment;
    RuntimeNative3DAsyncRenderProgressBuffer *progress;
    RuntimeNative3DAsyncRenderProgressSnapshot copied_snapshot;
    RuntimeNative3DAsyncRenderProgressRect rect = {2, 1, 3, 2};
    RuntimeNative3DAsyncRenderProgressRect invalid_rect = {7, 5, 2, 2};
    uint8_t host[8u * 6u * 4u];
    uint8_t copied[3u * 2u * 4u];
    size_t required = 0u;
    uint64_t digest = UINT64_C(1469598103934665603);
    FILE *artifact;
    size_t i;

    if (RuntimeNative3DAsyncRender_AssessSnapshot(NULL).readiness !=
        RUNTIME_NATIVE_3D_ASYNC_RENDER_BLOCKED_INVALID_SNAPSHOT)
        return 11;
    rt_stageg_trace("b3_invalid", "null_snapshot", 0u);
    if (!rt_stageg_build_snapshot(&base)) return 12;

    snapshot = base;
    snapshot.generationBound = false;
    if (!expect_readiness(&snapshot,
                          RUNTIME_NATIVE_3D_ASYNC_RENDER_BLOCKED_GENERATION_UNBOUND))
        return 13;
    snapshot = base;
    snapshot.cancelTokenBound = false;
    if (!expect_readiness(&snapshot,
                          RUNTIME_NATIVE_3D_ASYNC_RENDER_BLOCKED_CANCEL_UNBOUND))
        return 14;
    rt_stageg_trace("b3_identity", "generation_cancel_boundaries", 0u);

    snapshot = base;
    snapshot.preparedFrameBound = false;
    if (!expect_readiness(&snapshot,
                          RUNTIME_NATIVE_3D_ASYNC_RENDER_BLOCKED_PREPARED_FRAME_UNBOUND))
        return 15;
    snapshot = base;
    snapshot.sceneAccelerationBound = false;
    if (!expect_readiness(&snapshot,
                          RUNTIME_NATIVE_3D_ASYNC_RENDER_BLOCKED_ACCELERATION_UNBOUND))
        return 16;
    snapshot = base;
    snapshot.traceRoute = RUNTIME_RAY_3D_TRACE_ROUTE_FLATTENED_BVH;
    if (!expect_readiness(&snapshot,
                          RUNTIME_NATIVE_3D_ASYNC_RENDER_BLOCKED_NON_TLAS_ROUTE))
        return 17;
    rt_stageg_trace("b3_scene", "prepared_acceleration_route", 0u);

    snapshot = base;
    snapshot.volumeFrameSelectionDynamic = true;
    if (!expect_readiness(&snapshot,
                          RUNTIME_NATIVE_3D_ASYNC_RENDER_BLOCKED_DYNAMIC_VOLUME))
        return 18;
    snapshot = base;
    snapshot.waterSurfaceFrameSelectionDynamic = true;
    if (!expect_readiness(&snapshot,
                          RUNTIME_NATIVE_3D_ASYNC_RENDER_BLOCKED_DYNAMIC_WATER))
        return 19;
    rt_stageg_trace("b3_dynamic", "volume_water_rejected", 0u);

    assessment = RuntimeNative3DAsyncRender_AssessSnapshot(&base);
    if (!assessment.ready || !assessment.requiresExclusiveRenderContext ||
        assessment.readiness != RUNTIME_NATIVE_3D_ASYNC_RENDER_READY_EXCLUSIVE_SINGLE_JOB)
        return 20;
    digest = rt_stageg_snapshot_digest(&base);
    rt_stageg_trace("b3_ready", "exclusive_single_job", digest);

    progress = RuntimeNative3DAsyncRenderProgressBuffer_Create();
    if (!progress) return 21;
    for (i = 0u; i < sizeof(host); ++i) host[i] = (uint8_t)((i * 17u + 3u) & 0xffu);
    if (RuntimeNative3DAsyncRenderProgressBuffer_PublishDirtyRectABGR(
            progress, 77u, host, 8, 6, invalid_rect))
        return 22;
    if (!RuntimeNative3DAsyncRenderProgressBuffer_PublishDirtyRectABGR(
            progress, 77u, host, 8, 6, rect))
        return 23;
    rt_stageg_trace("b3_published", "dirty_rect_3x2", digest);

    memset(copied, 0, sizeof(copied));
    if (!RuntimeNative3DAsyncRenderProgressBuffer_CopyLatest(
            progress, 77u, &copied_snapshot, copied, sizeof(copied), &required) ||
        !copied_snapshot.valid || copied_snapshot.staleGeneration ||
        copied_snapshot.sequence != 1u || required != sizeof(copied))
        return 24;
    if (memcmp(copied, host + ((1u * 8u + 2u) * 4u), 12u) != 0 ||
        memcmp(copied + 12u, host + ((2u * 8u + 2u) * 4u), 12u) != 0)
        return 25;
    digest = rt_stageg_hash_bytes(digest, copied, sizeof(copied));
    rt_stageg_trace("b3_copied", "deep_copy_exact", digest);

    if (RuntimeNative3DAsyncRenderProgressBuffer_CopyLatest(
            progress, 78u, &copied_snapshot, copied, sizeof(copied), &required) ||
        !copied_snapshot.valid || !copied_snapshot.staleGeneration)
        return 26;
    rt_stageg_trace("b3_stale", "generation_rejected", digest);
    RuntimeNative3DAsyncRenderProgressBuffer_Reset(progress);
    if (RuntimeNative3DAsyncRenderProgressBuffer_CopyLatest(
            progress, 77u, &copied_snapshot, copied, sizeof(copied), &required))
        return 27;
    rt_stageg_trace("b3_reset", "progress_cleared", digest);

    artifact = fopen("progress.abgr", "wb");
    if (!artifact || fwrite(copied, 1u, sizeof(copied), artifact) != sizeof(copied) ||
        fclose(artifact) != 0)
        return 28;
    artifact = fopen("interaction.canonical", "wb");
    if (!artifact) return 29;
    fprintf(artifact,
            "schema=ray_tracing_stage_g_async_v1\nreadiness=ready_exclusive_single_job\n"
            "generation=77\nsequence=1\nhost=8x6\nrect=2,1,3,2\nbytes=%zu\n"
            "stale_rejected=1\nreset_cleared=1\ndigest=%016llx\n",
            sizeof(copied), (unsigned long long)digest);
    if (fclose(artifact) != 0) return 30;
    rt_stageg_trace("b3_canonical", "pixel_and_state_artifacts", digest);
    RuntimeNative3DAsyncRenderProgressBuffer_Destroy(progress);
    rt_stageg_trace("b3_shutdown", "progress_destroyed", 0u);
    return 0;
}
