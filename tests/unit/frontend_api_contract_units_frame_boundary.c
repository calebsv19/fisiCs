#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fisics_frontend.h"
typedef struct {const char *name,*old_text,*new_text,*definitions,*body;int code;} Case;
static const Case cases[]={
    {"pivot_class","original.rr[r][r];","original.tt[r][r];","","",4103},
    {"condense_tt","original.tr[i][r]*(original.rt[r][j]/pivot);","original.rr[i%2][r]*(original.rt[r][j]/pivot);","","",4102},
    {"condense_tr","original.tr[i][r]*(original.rr[r][j]/pivot);","original.tr[i][r]*(original.rr[r][j]/original.tt[0][0]);","","",4102},
    {"condense_rt","original.rr[i][r]*(original.rt[r][j]/pivot);","original.tr[i][r]*(original.rt[r][j]/pivot);","","",4102},
    {"condense_rr","original.rr[i][r]*(original.rr[r][j]/pivot);","original.tr[i][r]*(original.rr[r][j]/pivot);","","",4102},
    {"transform_tt","R[a][i]*original.tt[a][b]*R[b][j];","R[a][i]*original.rr[a%2][b%2]*R[b][j];","","",4101},
    {"transform_tr","R[a][i]*original.tr[a][j];","R[a][i]*original.tt[a][j];","","",4101},
    {"transform_rt","original.rt[i][a]*R[a][j];","original.rr[i][a%2]*R[a][j];","","",4101},
    {"transform_rr","out->rr[i][j]=original.rr[i][j];","out->rr[i][j]=original.tt[i][j];","","",4103},
    {"coordinate_class","out->translation[a]=basis->cosine*original.translation[a]+basis->sine*original.translation[a+1];","out->translation[a]=basis->cosine*original.rotation[end];","","",4103},
    {"force_class","out->force[a]=basis->cosine*original.force[a]-basis->sine*original.force[a+1];","out->force[a]=basis->cosine*original.moment[end];","","",4103},
    {"moment_class","out->moment[end]=original.moment[end];","out->moment[end]=original.force[end];","","",4103},
    {"complete_boundary","","","","FrameLocalStiffness k,out;FrameLocalBasis b;FrameDense6 d;frame_local_release(&k,1,0,&out);frame_local_transform(&out,&b,&out);frame_dense_pack(&out,&d);frame_dense_restore_assumed_si(&d,&k);",0},
    {"basis_length","","","","FrameLocalBasis b;FrameLocalCoordinates q;b.cosine=q.translation[0];",4103},
    {"restored_checks_resume","","","","FrameDense6 d;FrameLocalStiffness k;frame_dense_restore_assumed_si(&d,&k);k.tt[0][0]=k.rr[0][0];",4103},
    {"argument","","","void take(double x [[fisics::dim(force)]]);","FrameLocalStiffness k;take(k.rr[0][0]);",4112},
    {"return","","","double [[fisics::dim(force)]] f(FrameLocalLoads *p){return p->moment[0];}","",4113},
    {"translation_units","","","","FrameLocalCoordinates q;double ft [[fisics::dim(length)]] [[fisics::unit(foot)]]=1;q.translation[0]=ft;",4107},
    {"raw_restore_unchecked","","","","FrameDense6 d;FrameLocalStiffness k;d.value[0][0]=d.value[2][2];frame_dense_restore_assumed_si(&d,&k);",0},
    {"same_dimension_frame_identity","","","","FrameLocalBasis local,world;local=world;",0},
};
int main(void) {
    FILE *file=fopen("tests/fixtures/solver_physics/frame_boundary.h","rb");
    if(!file)return 1;
    if(fseek(file,0,SEEK_END)!=0)return 1;
    long size=ftell(file);if(size<0||size>65536)return 1;
    rewind(file);char *base=calloc((size_t)size+1,1);
    if(!base||fread(base,1,(size_t)size,file)!=(size_t)size)return 1;
    fclose(file);int failures=0;
    for(size_t i=0;i<sizeof(cases)/sizeof(cases[0]);++i) {
        char source[98304];const Case *c=&cases[i];
        const char *at=c->old_text[0]?strstr(base,c->old_text):NULL;
        if(c->old_text[0]&&(!at||strstr(at+strlen(c->old_text),c->old_text)))return 1;
        if(at)snprintf(source,sizeof(source),"%.*s%s%s\n%s\nint main(void){%s return 0;}\n",
            (int)(at-base),base,c->new_text,at+strlen(c->old_text),c->definitions,c->body);
        else snprintf(source,sizeof(source),"%s\n%s\nint main(void){%s return 0;}\n",base,c->definitions,c->body);
        FisicsFrontendOptions options={0};options.overlay_features=FISICS_OVERLAY_PHYSICS_UNITS;
        FisicsAnalysisResult result={0};
        int ok=fisics_analyze_buffer("tests/fixtures/solver_physics/frame_boundary_contract.c",source,strlen(source),&options,&result);
        size_t errors=0,matching=0;
        for(size_t j=0;j<result.diag_count;++j){
            const FisicsDiagnostic *d=&result.diagnostics[j];
            if(d->severity_id!=FISICS_DIAG_SEVERITY_ERROR)continue;
            ++errors;
            if(d->code_id==c->code&&d->category_id==FISICS_DIAG_CATEGORY_EXTENSION&&d->line>0&&d->column>0)++matching;
            else fprintf(stderr,"%s unexpected %d: %s\n",c->name,d->code_id,d->message);
        }
        size_t expected=c->code?1:0;
        if(!ok||errors!=expected||matching!=errors){fprintf(stderr,"FAIL %s: analyzed=%d errors=%zu expected=%zu\n",c->name,ok,errors,expected);++failures;}
        else printf("PASS %s\n",c->name);
        fisics_free_analysis_result(&result);
    }
    free(base);printf("Frame-boundary frontend contracts: %zu cases, %d failures\n",sizeof(cases)/sizeof(cases[0]),failures);
    return failures?1:0;
}
