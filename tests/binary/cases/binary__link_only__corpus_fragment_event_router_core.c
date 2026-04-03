typedef struct {
    unsigned topic;
    int payload;
} Event;

typedef int (*event_fn)(const Event *);

event_fn handlers_lookup(unsigned topic);

event_fn router_pick(unsigned topic) {
    return handlers_lookup(topic);
}

int router_dispatch(const Event *ev) {
    event_fn fn = router_pick(ev->topic);
    if (!fn) return -1;
    return fn(ev);
}
