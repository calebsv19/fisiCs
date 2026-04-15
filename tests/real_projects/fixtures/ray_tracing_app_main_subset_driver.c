#include "ray_tracing/ray_tracing_app_main.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static int g_legacy_calls = 0;

static int ray_tracing_legacy_entry_stub(int argc, char **argv) {
    g_legacy_calls++;
    if (argc >= 2 && argv && argv[1] && strcmp(argv[1], "--smoke") == 0) {
        return 0;
    }
    return 3;
}

static bool ray_tracing_true_callback(void *user_data) {
    int *counter = (int *)user_data;
    if (counter) {
        (*counter)++;
    }
    return true;
}

int main(void) {
    int ok = 1;
    int events_calls = 0;
    int update_calls = 0;
    int route_calls = 0;
    int submit_calls = 0;
    int submit_post_loop_calls = 0;
    int submit_post_shutdown_calls = 0;

    char arg0[] = "ray_tracing";
    char arg1[] = "--smoke";
    char *argv[] = {arg0, arg1, NULL};

    RayTracingFrameEventsRequest events_req = {ray_tracing_true_callback, &events_calls};
    RayTracingFrameEventsOutcome events_out = {0};
    RayTracingFrameUpdateRequest update_req = {ray_tracing_true_callback, &update_calls};
    RayTracingFrameUpdateOutcome update_out = {0};
    RayTracingFrameRouteRequest route_req = {ray_tracing_true_callback, &route_calls};
    RayTracingFrameRouteOutcome route_out = {0};
    RayTracingRenderSubmitRequest submit_req = {ray_tracing_true_callback, &submit_calls};
    RayTracingRenderSubmitOutcome submit_out = {0};
    RayTracingRenderSubmitRequest submit_post_loop_req = {
        ray_tracing_true_callback, &submit_post_loop_calls};
    RayTracingRenderSubmitOutcome submit_post_loop_out = {0};
    RayTracingRenderSubmitRequest submit_post_shutdown_req = {
        ray_tracing_true_callback, &submit_post_shutdown_calls};
    RayTracingRenderSubmitOutcome submit_post_shutdown_out = {0};

    ray_tracing_app_set_legacy_entry(ray_tracing_legacy_entry_stub);
    if (!ray_tracing_app_bootstrap()) ok = 0;
    if (!ray_tracing_app_config_load()) ok = 0;
    if (!ray_tracing_app_state_seed()) ok = 0;
    if (!ray_tracing_app_subsystems_init()) ok = 0;
    if (!ray_tracing_runtime_start()) ok = 0;

    if (!ray_tracing_app_frame_events(&events_req, &events_out)) ok = 0;
    if (!events_out.accepted_by_wrapper || !events_out.handled) ok = 0;
    if (!ray_tracing_app_frame_update(&update_req, &update_out)) ok = 0;
    if (!update_out.accepted_by_wrapper || !update_out.updated) ok = 0;
    if (!ray_tracing_app_frame_route(&route_req, &route_out)) ok = 0;
    if (!route_out.accepted_by_wrapper || !route_out.routed) ok = 0;
    if (!ray_tracing_app_render_submit(&submit_req, &submit_out)) ok = 0;
    if (!submit_out.accepted_by_wrapper || !submit_out.submitted) ok = 0;

    if (ray_tracing_app_run_loop() != 0) ok = 0;
    if (!ray_tracing_app_render_submit(&submit_post_loop_req, &submit_post_loop_out)) ok = 0;
    if (!submit_post_loop_out.accepted_by_wrapper || !submit_post_loop_out.submitted) ok = 0;

    ray_tracing_app_shutdown();

    if (ray_tracing_app_render_submit(&submit_post_shutdown_req, &submit_post_shutdown_out)) ok = 0;
    if (submit_post_shutdown_out.accepted_by_wrapper || submit_post_shutdown_out.submitted) ok = 0;

    ray_tracing_app_set_legacy_entry(ray_tracing_legacy_entry_stub);
    if (ray_tracing_app_main(2, argv) != 0) ok = 0;

    printf("legacy_calls=%d callbacks=%d,%d,%d,%d post_loop=%d post_shutdown=%d ok=%d\n",
           g_legacy_calls,
           events_calls,
           update_calls,
           route_calls,
           submit_calls,
           submit_post_loop_calls,
           submit_post_shutdown_calls,
           ok);
    return ok ? 0 : 1;
}
