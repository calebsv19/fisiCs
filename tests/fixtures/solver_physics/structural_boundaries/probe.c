#include "core_structural_frame_boundaries.h"
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
static void vector(const char *name,const double *v,size_t n) {
    printf("%s",name);for(size_t i=0;i<n;++i)printf(" %.17g",v[i]);puts("");
}
int main(void) {
    const double raw[12]={0,0,0,1,2,3,0,0,0,4,5,6};
    CsFrameWorldDofs map=cs_frame_world_dofs_assumed_order(3,9);
    CsFrameWorldCoordinates world;cs_frame_world_coordinates_restore_assumed_si(raw,&map,&world);
    const CsFrameMemberBasis bases[4]={{1,0},{0,1},{-1,0},{.6,.8}};
    const double load[6]={10,20,30,40,50,60};
    CsFrameLocalLoads local_load;cs_physical_loads_restore_assumed_si(load,&local_load.value);
    for(size_t b=0;b<4;++b) {
        CsFrameLocalCoordinates local;CsFrameWorldLoads global;
        cs_frame_world_to_local_coordinates(&bases[b],&world,&local);
        cs_frame_local_to_world_loads(&bases[b],&local_load,&global);
        double q[6],f[6];cs_frame_local_coordinates_pack_assumed_order(&local,q);
        cs_physical_loads_pack(&global.value,f);
        printf("B %zu",b);for(size_t i=0;i<6;++i)printf(" %.17g",q[i]);
        for(size_t i=0;i<6;++i)printf(" %.17g",f[i]);
        double world_work=0,local_work=0;
        for(size_t i=0;i<6;++i){world_work+=raw[map.index[i]]*f[i];local_work+=q[i]*load[i];}
        printf(" %.17g %.17g\n",world_work,local_work);
        CsFrameWorldLineLoad wl={10,20};CsFrameLocalLineLoad ll;
        cs_frame_world_to_local_line_load(&bases[b],&wl,&ll);
        double line[2]={ll.x,ll.y};vector("LINE",line,2);
    }
    CsFrameLocalMatrix diagonal={0};for(size_t i=0;i<6;++i)diagonal.value[i][i]=(double)i+1;
    CsFrameWorldMatrix rotated;if(!cs_frame_local_to_world_stiffness(&bases[3],&diagonal,&rotated))return 1;
    vector("K",&rotated.value[0][0],36);
    CsPhysicalSection section={120,2,3,5};CsPhysicalStiffness stiffness;
    cs_physical_build(&section,&stiffness);
    const double rigid[6]={-.375,.25,.125,-.875,.625,.125};
    CsFrameWorldDofs zero=cs_frame_world_dofs_assumed_order(0,3);
    cs_frame_world_coordinates_restore_assumed_si(rigid,&zero,&world);
    CsFrameLocalCoordinates local;cs_frame_world_to_local_coordinates(&bases[3],&world,&local);
    CsPhysicalLoads internal;cs_physical_apply(&stiffness,&local.value,&internal);
    double force[6];cs_physical_loads_pack(&internal,force);vector("RIGID",force,6);
    CsPhysicalCoordinates translation={{2,-3,2,-3},{0,0}};
    cs_physical_apply(&stiffness,&translation,&internal);cs_physical_loads_pack(&internal,force);vector("TRANSLATION",force,6);
    CsFrameElement element={0};CsFrameLocalMatrix original;
    cs_frame_local_matrix_pack_assumed_si(&stiffness,&original);
    memcpy(element.local_stiffness,original.value,sizeof(element.local_stiffness));
    CsPhysicalLineLoad line={0,12,5};CsFrameLocalLoads equivalent;
    cs_physical_equivalent_load(&line,&equivalent.value);
    for(unsigned mask=0;mask<4;++mask) {
        CsFrameLocalMatrix condensed=original;CsFrameLocalLoads effective=equivalent;
        element.release_a=(mask&1)!=0;element.release_b=(mask&2)!=0;
        cs_frame_local_release_condense(&condensed,element.release_a,element.release_b);
        double rotations[2];cs_frame_released_rotations_assumed_si(element.local_stiffness,
            element.release_a,element.release_b,&equivalent,rotations);
        cs_frame_local_release_load_assumed_si(&element,&effective);
        printf("RELEASE %u",mask);for(size_t i=0;i<36;++i)printf(" %.17g",(&condensed.value[0][0])[i]);
        for(size_t i=0;i<2;++i)printf(" %.17g",rotations[i]);
        cs_physical_loads_pack(&effective.value,force);for(size_t i=0;i<6;++i)printf(" %.17g",force[i]);puts("");
    }
    CsFrameWorkspace *w=calloc(1,sizeof(*w));if(!w)return 1;
    for(size_t i=0;i<CS_FRAME_DOF;++i)w->compiled.free_index[i]=-1;
    const int free_map[6]={0,1,2,0,3,-1};for(size_t i=0;i<6;++i)w->compiled.free_index[map.index[i]]=free_map[i];
    w->compiled.free_count=4;
    CsFrameWorldMatrix matrix={0};for(size_t i=0;i<6;++i)matrix.value[i][i]=(double)i+1;
    matrix.value[0][3]=matrix.value[3][0]=7;
    CoreStructuralDiagnostic diagnostic={0};
    if(cs_frame_scatter_world_stiffness_assumed_si(&matrix,&map,w,"test",&diagnostic)!=CORE_STRUCTURAL_OK)return 1;
    double reduced[4];for(size_t i=0;i<4;++i)reduced[i]=w->reduced[i*CS_FRAME_DOF+i];vector("REDUCED",reduced,4);
    CsFrameWorldLoads global;cs_frame_local_to_world_loads(&bases[3],&local_load,&global);
    if(cs_frame_scatter_world_load_assumed_si(&global,&map,w,"test",&diagnostic)!=CORE_STRUCTURAL_OK)return 1;
    vector("RHS",w->rhs,4);
    cs_frame_restore_world_solution_assumed_si(w->compiled.free_index,w->rhs,12,w->displacement);
    double restored[6];for(size_t i=0;i<6;++i)restored[i]=w->displacement[map.index[i]];vector("RESTORED",restored,6);
    memset(w,0,sizeof(*w));w->compiled.free_count=2;w->geometry_rows=2;
    w->geometry[0]=1;w->geometry[CS_FRAME_DOF+1]=1;
    w->reduced[0]=4;w->reduced[1]=1;w->reduced[CS_FRAME_DOF]=1;w->reduced[CS_FRAME_DOF+1]=3;
    w->rhs[0]=6;w->rhs[1]=7;CoreStructuralFrameResult *result=calloc(1,sizeof(*result));if(!result)return 1;
    if(cs_frame_linear(w,result,&diagnostic)!=CORE_STRUCTURAL_OK)return 1;
    vector("SOLVED",w->rhs,2);
    w->reduced[0]=-1;
    if(cs_frame_linear(w,result,&diagnostic)!=CORE_STRUCTURAL_ILL_CONDITIONED)return 1;
    diagonal.value[0][0]=NAN;if(cs_frame_local_to_world_stiffness(&bases[3],&diagonal,&rotated))return 1;
    memset(w,0,sizeof(*w));for(size_t i=0;i<CS_FRAME_DOF;++i)w->compiled.free_index[i]=-1;
    w->full[map.index[0]*CS_FRAME_DOF+map.index[0]]=DBL_MAX;matrix.value[0][0]=DBL_MAX;
    if(cs_frame_scatter_world_stiffness_assumed_si(&matrix,&map,w,"test",&diagnostic)!=CORE_STRUCTURAL_NUMERICAL_FAILURE)return 1;
    global.value.force[0]=INFINITY;
    if(cs_frame_scatter_world_load_assumed_si(&global,&map,w,"test",&diagnostic)!=CORE_STRUCTURAL_NUMERICAL_FAILURE)return 1;
    puts("REFUSALS 4");free(result);free(w);return 0;
}
