typedef struct {
    unsigned topic;
    int payload;
} Event;

typedef int (*event_fn)(const Event *);

static int on_ping(const Event *ev) { return ev->payload + 1; }
static int on_pong(const Event *ev) { return ev->payload - 1; }
static int on_data(const Event *ev) { return ev->payload * 2; }

event_fn handlers_lookup(unsigned topic) {
    if (topic == 0u) return on_ping;
    if (topic == 1u) return on_pong;
    if (topic == 2u) return on_data;
    return (event_fn)0;
}
