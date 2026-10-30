#include "core_structural_dynamic_energy.h"
/* Thin ABI surface over the actual private helpers. Restoration is deliberately
 * not a strict scope; each selected helper body is qualified independently. */
double coupling_kinetic(const double v[3]) {
    const double m[3][3]={{4,0,.5},{0,8,-.25},{.5,-.25,2}};
    double result=0;
    for(unsigned i=0;i<3;i++)for(unsigned j=0;j<3;j++)
        result+=cs_de_kinetic(i,j,v[i],m[i][j],v[j]);
    return result;
}
double coupling_damping(const double old[3],const double now[3]) {
    const double m[3][3]={{4,0,.5},{0,8,-.25},{.5,-.25,2}};
    double result=0;
    for(unsigned i=0;i<3;i++)for(unsigned j=0;j<3;j++)
        result+=cs_de_damping(i,j,.125,.5,old[i],now[i],m[i][j],old[j],now[j]);
    return result;
}
