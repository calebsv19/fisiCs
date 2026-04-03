typedef struct {
    unsigned topic;
    int payload;
} Event;

typedef int (*event_fn)(const Event *);

event_fn router_pick(unsigned topic);
int router_dispatch(const Event *ev);

int main(void) {
    Event ev;
    ev.topic = 2u;
    ev.payload = 41;
    return router_dispatch(&ev) + (router_pick(1u) ? 0 : 1);
}
