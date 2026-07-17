#include "11__probe_wave89_realproj_request_callback_routing.h"

#include <stdio.h>

typedef struct Wave89Counter {
    int calls;
    int increment;
} Wave89Counter;

static bool wave89_events_callback(void *user_data) {
    Wave89Counter *counter = (Wave89Counter *)user_data;
    counter->calls += counter->increment;
    return true;
}

static bool wave89_update_callback(void *user_data) {
    Wave89Counter *counter = (Wave89Counter *)user_data;
    counter->calls += counter->increment;
    return true;
}

static bool wave89_route_callback(void *user_data) {
    Wave89Counter *counter = (Wave89Counter *)user_data;
    counter->calls += counter->increment;
    return true;
}

static bool wave89_submit_callback(void *user_data) {
    Wave89Counter *counter = (Wave89Counter *)user_data;
    counter->calls += counter->increment;
    return true;
}

int main(void) {
    Wave89Counter events_counter = {0, 1};
    Wave89Counter update_counter = {0, 2};
    Wave89Counter route_counter = {0, 3};
    Wave89Counter submit_counter = {0, 4};
    Wave89Counter post_counter = {0, 8};
    Wave89FrameEventsRequest events_request = {
        wave89_events_callback, &events_counter
    };
    Wave89FrameUpdateRequest update_request = {
        wave89_update_callback, &update_counter
    };
    Wave89FrameRouteRequest route_request = {
        wave89_route_callback, &route_counter
    };
    Wave89RenderSubmitRequest submit_request = {
        wave89_submit_callback, &submit_counter
    };
    Wave89RenderSubmitRequest post_request = {
        wave89_submit_callback, &post_counter
    };
    Wave89RequestOutcome events_outcome = {false, false};
    Wave89RequestOutcome update_outcome = {false, false};
    Wave89RequestOutcome route_outcome = {false, false};
    Wave89RequestOutcome submit_outcome = {false, false};
    Wave89RequestOutcome post_outcome = {true, true};
    bool events_ok;
    bool update_ok;
    bool route_ok;
    bool submit_ok;
    bool post_ok;
    int accepted;
    int total;

    wave89_request_runtime_start();
    events_ok = wave89_frame_events(&events_request, &events_outcome);
    update_ok = wave89_frame_update(&update_request, &update_outcome);
    route_ok = wave89_frame_route(&route_request, &route_outcome);
    submit_ok = wave89_render_submit(&submit_request, &submit_outcome);
    wave89_request_runtime_shutdown();
    post_ok = wave89_render_submit(&post_request, &post_outcome);

    accepted = events_outcome.accepted_by_wrapper
        + update_outcome.accepted_by_wrapper
        + route_outcome.accepted_by_wrapper
        + submit_outcome.accepted_by_wrapper;
    total = events_counter.calls + update_counter.calls
        + route_counter.calls + submit_counter.calls;

    printf("callbacks=%d,%d,%d,%d post=%d accepted=%d rejected=%d total=%d\n",
           events_counter.calls,
           update_counter.calls,
           route_counter.calls,
           submit_counter.calls,
           post_counter.calls,
           accepted,
           !post_ok && !post_outcome.accepted_by_wrapper && !post_outcome.handled,
           total);

    return events_ok && update_ok && route_ok && submit_ok
        && events_outcome.handled && update_outcome.handled
        && route_outcome.handled && submit_outcome.handled
        && !post_ok && !post_outcome.accepted_by_wrapper && !post_outcome.handled
        && total == 10 ? 0 : 1;
}
