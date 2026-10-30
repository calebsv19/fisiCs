/* Report-only qualification. Ordinary C control builds use the portability header. */
#include <fisics/extensions.h>
#include <math.h>
#ifdef STRIP_METADATA
#define D(x)
#else
#define D(x) FISICS_DIM(x)
#endif
struct Body { double position[2] D(length); double velocity[2] D(speed); };
double travel(double speed D(speed),double dt D(time)) D(length) {
    double distance D(length)=speed*dt;
    return distance;
}
double field_update(struct Body* body,double dt D(time)) D(length) {
    double dx D(length)=body->velocity[0]*dt;
#ifdef BAD_UPDATE
    body->position[0]=dt;
#else
    body->position[0]=body->position[0]+dx;
#endif
    return body->position[0];
}
extern double declared_source(void) D(force);
extern double raw_source(void);
double trust_path(void) D(force) {
    double force D(force)=declared_source();
    return force;
}
double missing_path(void) D(force) {
    double force D(force)=raw_source();
    return force;
}
double compound(double force D(force),double scale) D(force) {
    force*=2;
    force+=force;
    force*=scale;
    return force;
}
struct Length { double x D(length); };
struct Time { double x D(time); };
double record_identity(struct Length* a,struct Time* b) D(length) {
    double distance D(length)=a->x;
    double dt D(time)=b->x;
    (void)dt;
    return distance;
}
void branch_scope(double x D(length),double dt D(time)) {
    if(1) { x=x; } else {
#ifdef BAD_BRANCH
        x=dt;
#else
        x=x;
#endif
    }
    for(int i=0;i<2;i++) { x=x; }
}
void empty_scope(void) {}

double norm(double x D(length),double y D(length)) D(length) {
    double magnitude D(length)=hypot(x,y);
    return fabs(magnitude);
}

extern double accepted_force(double value D(force)) D(force);
double arguments(double force D(force),double dt D(time)) D(force) {
#ifdef BAD_ARGUMENT
    return accepted_force(dt);
#else
    return accepted_force(force);
#endif
}
