typedef enum ProbeViewAxis {
    PROBE_VIEW_XY = 0,
    PROBE_VIEW_YZ = 1,
    PROBE_VIEW_XZ = 2
} ProbeViewAxis;

typedef struct ProbeViewPlane {
    ProbeViewAxis axis;
    float offset;
} ProbeViewPlane;

typedef struct ProbeViewContext {
    ProbeViewPlane plane;
    int pad;
} ProbeViewContext;

void probe_mixed_view_set(ProbeViewPlane plane);
void probe_mixed_view_set_parts(ProbeViewAxis axis, float offset);
ProbeViewPlane probe_mixed_view_get(void);
float probe_mixed_view_offset(void);
ProbeViewContext probe_mixed_view_context(void);
