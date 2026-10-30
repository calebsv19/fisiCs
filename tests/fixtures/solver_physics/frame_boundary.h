/* Bounded fixture for local releases, basis changes, and dense exchange.
 * Inputs are admitted finite SI coefficients; the release pivot is positive.
 * Basis cosine/sine must be finite and normalized. No frame-ID checking here.
 * Output may alias input. Releases are local and precede transformation.
 * Dense restore is an explicit assumed-SI trust boundary, not inferred units.
 */
#ifndef FISICS_TEST_FRAME_BOUNDARY_H
#define FISICS_TEST_FRAME_BOUNDARY_H
#include "frame_local.h"
#include <stdbool.h>

typedef struct FrameLocalBasis {
    double cosine FISICS_DIM(scalar);
    double sine FISICS_DIM(scalar);
} FrameLocalBasis;
typedef struct FrameDense6 { double value[6][6]; } FrameDense6;

static void frame_local_release(const FrameLocalStiffness *input, bool release_a,
    bool release_b, FrameLocalStiffness *out) {
    FrameLocalStiffness original = *input;
    *out = original;
    if (!release_a && !release_b) return;
    double zero_tt FISICS_DIM(force/length)=0;
    double zero_coupling FISICS_DIM(force)=0;
    double zero_rr FISICS_DIM(force*length)=0;
    if (release_a && release_b) {
        for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)
            if(i%2 || j%2)out->tt[i][j]=zero_tt;
        for(unsigned i=0;i<4;++i)for(unsigned j=0;j<2;++j)out->tr[i][j]=zero_coupling;
        for(unsigned i=0;i<2;++i)for(unsigned j=0;j<4;++j)out->rt[i][j]=zero_coupling;
        for(unsigned i=0;i<2;++i)for(unsigned j=0;j<2;++j)out->rr[i][j]=zero_rr;
        return;
    }
    unsigned r=release_a?0:1;
    double pivot FISICS_DIM(force*length)=original.rr[r][r];
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)
        out->tt[i][j]-=original.tr[i][r]*(original.rt[r][j]/pivot);
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<2;++j)
        if(j!=r)out->tr[i][j]-=original.tr[i][r]*(original.rr[r][j]/pivot);
    for(unsigned i=0;i<2;++i)for(unsigned j=0;j<4;++j)
        if(i!=r)out->rt[i][j]-=original.rr[i][r]*(original.rt[r][j]/pivot);
    for(unsigned i=0;i<2;++i)for(unsigned j=0;j<2;++j)
        if(i!=r && j!=r)out->rr[i][j]-=original.rr[i][r]*(original.rr[r][j]/pivot);
    for(unsigned i=0;i<4;++i){out->tr[i][r]=zero_coupling;out->rt[r][i]=zero_coupling;}
    for(unsigned i=0;i<2;++i){out->rr[i][r]=zero_rr;out->rr[r][i]=zero_rr;}
}

/* q_local = R q_global; translations rotate, planar rotations are unchanged. */
static void frame_translation_basis(const FrameLocalBasis *basis,
    double R[4][4] FISICS_DIM(scalar)) {
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)R[i][j]=0;
    for(unsigned end=0;end<2;++end){unsigned a=2*end;
        R[a][a]=basis->cosine;R[a][a+1]=basis->sine;
        R[a+1][a]=-basis->sine;R[a+1][a+1]=basis->cosine;
    }
}
static void frame_local_transform(const FrameLocalStiffness *input,
    const FrameLocalBasis *basis, FrameLocalStiffness *out) {
    FrameLocalStiffness original=*input;
    double R[4][4] FISICS_DIM(scalar);frame_translation_basis(basis,R);
    double zero_tt FISICS_DIM(force/length)=0;
    double zero_coupling FISICS_DIM(force)=0;
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j){
        out->tt[i][j]=zero_tt;
        for(unsigned a=0;a<4;++a)for(unsigned b=0;b<4;++b)
            out->tt[i][j]+=R[a][i]*original.tt[a][b]*R[b][j];
    }
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<2;++j){
        out->tr[i][j]=zero_coupling;
        for(unsigned a=0;a<4;++a)out->tr[i][j]+=R[a][i]*original.tr[a][j];
    }
    for(unsigned i=0;i<2;++i)for(unsigned j=0;j<4;++j){
        out->rt[i][j]=zero_coupling;
        for(unsigned a=0;a<4;++a)out->rt[i][j]+=original.rt[i][a]*R[a][j];
    }
    for(unsigned i=0;i<2;++i)for(unsigned j=0;j<2;++j)out->rr[i][j]=original.rr[i][j];
}
static void frame_coordinates_to_local(const FrameLocalCoordinates *input,
    const FrameLocalBasis *basis, FrameLocalCoordinates *out) {
    FrameLocalCoordinates original=*input;
    for(unsigned end=0;end<2;++end){unsigned a=2*end;
        out->translation[a]=basis->cosine*original.translation[a]+basis->sine*original.translation[a+1];
        out->translation[a+1]=-basis->sine*original.translation[a]+basis->cosine*original.translation[a+1];
        out->rotation[end]=original.rotation[end];
    }
}
static void frame_loads_to_global(const FrameLocalLoads *input,
    const FrameLocalBasis *basis, FrameLocalLoads *out) {
    FrameLocalLoads original=*input;
    for(unsigned end=0;end<2;++end){unsigned a=2*end;
        out->force[a]=basis->cosine*original.force[a]-basis->sine*original.force[a+1];
        out->force[a+1]=basis->sine*original.force[a]+basis->cosine*original.force[a+1];
        out->moment[end]=original.moment[end];
    }
}

/* Exact row-major exchange in ux_a,uy_a,rz_a,ux_b,uy_b,rz_b order.
 * Raw storage deliberately has no universal dimension or per-slot inference. */
static void frame_dense_pack(const FrameLocalStiffness *k, FrameDense6 *dense) {
    const unsigned t[4]={0,1,3,4}, r[2]={2,5};
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)dense->value[t[i]][t[j]]=k->tt[i][j];
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<2;++j)dense->value[t[i]][r[j]]=k->tr[i][j];
    for(unsigned i=0;i<2;++i)for(unsigned j=0;j<4;++j)dense->value[r[i]][t[j]]=k->rt[i][j];
    for(unsigned i=0;i<2;++i)for(unsigned j=0;j<2;++j)dense->value[r[i]][r[j]]=k->rr[i][j];
}
/* Caller asserts the same SI representation and ordering. This function assigns
 * declared types to unknown raw values; it does not validate their provenance. */
static void frame_dense_restore_assumed_si(const FrameDense6 *dense, FrameLocalStiffness *k) {
    const unsigned t[4]={0,1,3,4}, r[2]={2,5};
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)k->tt[i][j]=dense->value[t[i]][t[j]];
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<2;++j)k->tr[i][j]=dense->value[t[i]][r[j]];
    for(unsigned i=0;i<2;++i)for(unsigned j=0;j<4;++j)k->rt[i][j]=dense->value[r[i]][t[j]];
    for(unsigned i=0;i<2;++i)for(unsigned j=0;j<2;++j)k->rr[i][j]=dense->value[r[i]][r[j]];
}
#endif
