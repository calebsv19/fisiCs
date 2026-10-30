/* Included by the independent three-body oracle; publish only after tick success. */
static unsigned audit_tick,audit_case,audit_count;
static char audit_rows[2][2048];
void ce_observe(const PsCoulombWork *w,double before,double after,double loss,double dp,unsigned n,const PsCoulombHit *hits,const double *oldv,const double *impulses){
 need(audit_count<2,"observer group cap");
 int len=snprintf(audit_rows[audit_count],sizeof(audit_rows[0]),"G %u %u %u %.17g %.17g %.17g %.17g %u",audit_case,audit_tick,audit_count,before,after,loss,dp,n);
 for(unsigned i=0;i<3;i++)len+=snprintf(audit_rows[audit_count]+len,sizeof(audit_rows[0])-(unsigned)len," %.17g %.17g %.17g",w->step.bodies[i].sphere.mass,oldv[i],w->step.bodies[i].sphere.v[0]);
 for(unsigned i=0;i<n;i++){const PsContactEvent *e=&w->trace.events[w->trace.count-n+i];len+=snprintf(audit_rows[audit_count]+len,sizeof(audit_rows[0])-(unsigned)len," %u %u %.17g %.17g %.17g",hits[i].a,hits[i].b,impulses[i],e->time_lo,e->time_hi);}
 need(len>0&&(unsigned)len<sizeof(audit_rows[0]),"observer buffer");audit_count++;
}
static void audit_publish(const PsRestStep *o,const PsCoulombTrace *t){
 printf("T %u %u %.17g %.17g %u %u",audit_case,audit_tick,o->energy_before,o->dissipation,t->count,audit_count);
 for(unsigned i=0;i<3;i++)printf(" %.17g %.17g %.17g",o->bodies[i].sphere.mass,o->bodies[i].sphere.x[0],o->bodies[i].sphere.v[0]);puts("");
 for(unsigned i=0;i<audit_count;i++)puts(audit_rows[i]);
}
