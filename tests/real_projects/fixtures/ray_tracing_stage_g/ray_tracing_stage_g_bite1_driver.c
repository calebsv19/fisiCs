#include "ray_tracing_stage_g_common.h"

int main(void) {
    RuntimeNative3DRenderRequestSnapshot snapshot;
    RuntimeNative3DRenderRequestSnapshot invalid;
    RuntimeNative3DRenderRequestSnapshotDesc bad;
    uint64_t digest;

    RuntimeNative3DRenderRequestSnapshot_Init(&snapshot);
    if (snapshot.valid || snapshot.traceRoute != RUNTIME_RAY_3D_TRACE_ROUTE_FLATTENED_BVH)
        return 11;
    rt_stageg_trace("b1_initialized", "default_flattened", 0u);

    if (!rt_stageg_build_snapshot(&snapshot)) return 12;
    digest = rt_stageg_snapshot_digest(&snapshot);
    rt_stageg_trace("b1_built", "production_snapshot", digest);
    if (!snapshot.samplingBound || !snapshot.resourceBudgetBound ||
        snapshot.outputWidth != 512 || snapshot.renderWidth != 1024)
        return 13;
    rt_stageg_trace("b1_dimensions", "output_render_host", digest);
    if (!snapshot.preparedFrameValid || snapshot.preparedTriangleCount != 48000u ||
        snapshot.materialCount != 11u || snapshot.enabledLightCount != 4u)
        return 14;
    rt_stageg_trace("b1_scene_state", "prepared_material_light", digest);
    if (!snapshot.sceneAccelerationBound ||
        snapshot.traceRoute != RUNTIME_RAY_3D_TRACE_ROUTE_TLAS_BLAS ||
        !snapshot.cancelTokenBound || snapshot.cancelGeneration != 77u)
        return 15;
    rt_stageg_trace("b1_handoff", "acceleration_cancel", digest);
    if (!snapshot.outputRootBound || !snapshot.summaryDestinationBound ||
        !snapshot.progressDestinationBound)
        return 16;
    rt_stageg_trace("b1_destinations", "portable_paths", digest);

    memset(&bad, 0, sizeof(bad));
    bad.outputWidth = 512;
    bad.outputHeight = 288;
    bad.renderWidth = 0;
    bad.renderHeight = 576;
    bad.hostWidth = 512;
    bad.hostHeight = 288;
    if (RuntimeNative3DRenderRequestSnapshot_Build(&invalid, &bad) || invalid.valid ||
        RuntimeNative3DRenderRequestSnapshot_Build(&invalid, NULL))
        return 17;
    rt_stageg_trace("b1_invalid", "null_and_zero_rejected", digest);

    if (!rt_stageg_write_snapshot("snapshot.canonical", &snapshot, digest)) return 18;
    rt_stageg_trace("b1_canonical", "artifact_written", digest);
    RuntimeNative3DRenderRequestSnapshot_Init(&snapshot);
    rt_stageg_trace("b1_shutdown", "snapshot_cleared", 0u);
    return 0;
}
