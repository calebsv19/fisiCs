#include <stdio.h>
#include <math.h>
double coupling_kinetic(const double [3]);
double coupling_damping(const double [3],const double [3]);
static double quadratic(double x,double y,double z){return 4*x*x+8*y*y+2*z*z+x*z-.5*y*z;}
int main(void) {
    unsigned checks=0;
    for(int a=-4;a<=4;a++)for(int b=-4;b<=4;b++)for(int c=-4;c<=4;c++) {
        double v[3]={a*.25,b*.25,c*.25};
        double old[3]={c*.125,a*.125,b*.125};
        double k=.5*quadratic(v[0],v[1],v[2]);
        double d=.015625*quadratic(old[0]+v[0],old[1]+v[1],old[2]+v[2]);
        double actual_k=coupling_kinetic(v),actual_d=coupling_damping(old,v);
        if(actual_k!=k || actual_d!=d || actual_k<0 || actual_d<0) {
            fprintf(stderr,"cross-block mismatch %d %d %d %.17g %.17g %.17g %.17g\n",a,b,c,actual_k,k,actual_d,d);return 1;
        }
        checks+=4;
    }
    printf("coupled SPD kinetic/damping: %u exact checks PASS\n",checks);return 0;
}
