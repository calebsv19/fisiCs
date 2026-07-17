#include "11__probe_wave89_realproj_request_callback_routing.h"

static bool wave89_runtime_active;

static bool wave89_dispatch(bool (*callback)(void *),
                            void *user_data,
                            Wave89RequestOutcome *outcome) {
    if (outcome) {
        outcome->accepted_by_wrapper = false;
        outcome->handled = false;
    }
    if (!wave89_runtime_active || !callback || !outcome) {
        return false;
    }
    outcome->accepted_by_wrapper = true;
    outcome->handled = callback(user_data);
    return true;
}

void wave89_request_runtime_start(void) {
    wave89_runtime_active = true;
}

void wave89_request_runtime_shutdown(void) {
    wave89_runtime_active = false;
}

bool wave89_frame_events(const Wave89FrameEventsRequest *request,
                         Wave89RequestOutcome *outcome) {
    return request && wave89_dispatch(request->events_fn, request->user_data, outcome);
}

bool wave89_frame_update(const Wave89FrameUpdateRequest *request,
                         Wave89RequestOutcome *outcome) {
    return request && wave89_dispatch(request->update_fn, request->user_data, outcome);
}

bool wave89_frame_route(const Wave89FrameRouteRequest *request,
                        Wave89RequestOutcome *outcome) {
    return request && wave89_dispatch(request->route_fn, request->user_data, outcome);
}

bool wave89_render_submit(const Wave89RenderSubmitRequest *request,
                          Wave89RequestOutcome *outcome) {
    return request && wave89_dispatch(request->submit_fn, request->user_data, outcome);
}
