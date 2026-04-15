#include "Layout/layout.h"

#include <math.h>
#include <stdio.h>

static int approx_eq(float a, float b) {
    float d = a - b;
    if (d < 0.0f) d = -d;
    return d < 1e-3f;
}

int main(void) {
    Scene3DSettings settings;
    Vec3 p = {150.0f, -120.0f, 25.0f};
    bool clamped = false;
    int ok = 1;
    ViewPlane vp;

    Layout_Scene3DSettings_SetDefaults(&settings);
    if (!Layout_SceneBounds3D_ClampPoint(&settings.bounds, &p, &clamped)) ok = 0;
    if (!clamped) ok = 0;
    if (!approx_eq(p.x, 100.0f) || !approx_eq(p.y, -100.0f) || !approx_eq(p.z, 25.0f)) ok = 0;

    Layout_ConstructionPlane3D_SetFromViewPlane(
        &settings.constructionPlane,
        (ViewPlane){ .axis = VIEW_PLANE_YZ, .offset = 7.0f }
    );
    vp = Layout_ConstructionPlane3D_ToViewPlane(&settings.constructionPlane);
    if (vp.axis != VIEW_PLANE_YZ || !approx_eq(vp.offset, 7.0f)) ok = 0;

    settings.constructionPlane.mode = CONSTRUCTION_PLANE_MODE_CUSTOM_FRAME;
    settings.constructionPlane.customFrame.origin = (Vec3){0.0f, 0.0f, 5.0f};
    settings.constructionPlane.customFrame.axisU = (Vec3){1.0f, 0.0f, 0.0f};
    settings.constructionPlane.customFrame.axisV = (Vec3){0.0f, 1.0f, 0.0f};
    settings.constructionPlane.customFrame.normal = (Vec3){0.0f, 0.0f, 1.0f};
    if (!Layout_ConstructionPlane3D_IsValid(&settings.constructionPlane)) ok = 0;
    vp = Layout_ConstructionPlane3D_ToViewPlane(&settings.constructionPlane);
    if (vp.axis != VIEW_PLANE_XY || !approx_eq(vp.offset, 5.0f)) ok = 0;

    printf("clamped=%d x=%.1f y=%.1f z=%.1f plane=%d offset=%.1f ok=%d\n",
           clamped ? 1 : 0,
           p.x,
           p.y,
           p.z,
           (int)vp.axis,
           vp.offset,
           ok);
    return ok ? 0 : 1;
}
