#ifndef FISICS_PROBE_WAVE89_REALPROJ_REQUEST_CALLBACK_ROUTING_H
#define FISICS_PROBE_WAVE89_REALPROJ_REQUEST_CALLBACK_ROUTING_H

#include <stdbool.h>

typedef bool (*Wave89FrameEventsFn)(void *user_data);
typedef bool (*Wave89FrameUpdateFn)(void *user_data);
typedef bool (*Wave89FrameRouteFn)(void *user_data);
typedef bool (*Wave89RenderSubmitFn)(void *user_data);

typedef struct Wave89FrameEventsRequest {
    Wave89FrameEventsFn events_fn;
    void *user_data;
} Wave89FrameEventsRequest;

typedef struct Wave89FrameUpdateRequest {
    Wave89FrameUpdateFn update_fn;
    void *user_data;
} Wave89FrameUpdateRequest;

typedef struct Wave89FrameRouteRequest {
    Wave89FrameRouteFn route_fn;
    void *user_data;
} Wave89FrameRouteRequest;

typedef struct Wave89RenderSubmitRequest {
    Wave89RenderSubmitFn submit_fn;
    void *user_data;
} Wave89RenderSubmitRequest;

typedef struct Wave89RequestOutcome {
    bool accepted_by_wrapper;
    bool handled;
} Wave89RequestOutcome;

void wave89_request_runtime_start(void);
void wave89_request_runtime_shutdown(void);
bool wave89_frame_events(const Wave89FrameEventsRequest *request,
                         Wave89RequestOutcome *outcome);
bool wave89_frame_update(const Wave89FrameUpdateRequest *request,
                         Wave89RequestOutcome *outcome);
bool wave89_frame_route(const Wave89FrameRouteRequest *request,
                        Wave89RequestOutcome *outcome);
bool wave89_render_submit(const Wave89RenderSubmitRequest *request,
                          Wave89RequestOutcome *outcome);

#endif
