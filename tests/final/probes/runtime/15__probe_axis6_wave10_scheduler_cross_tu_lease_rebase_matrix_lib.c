typedef struct S { unsigned int shard, lease, rebase, lane_a, lane_b; } S;
typedef struct A { unsigned int lease[4], rebase[4], lane_a[4], lane_b[4]; } A;
static unsigned int mix10(unsigned int h,unsigned int v){ h ^= v + 0x9e3779b9u + (h<<6) + (h>>2); return h; }
void seed10(S* out,unsigned int shard,unsigned int lease,unsigned int rebase,unsigned int lane_a,unsigned int lane_b){ out->shard=shard%4u; out->lease=lease; out->rebase=rebase; out->lane_a=lane_a; out->lane_b=lane_b; }
void encode10(const S* s,unsigned int wire[5]){ wire[0]=s->shard^0x65u; wire[1]=s->lease^0x87u; wire[2]=s->rebase^0xa9u; wire[3]=s->lane_a^0xcbu; wire[4]=s->lane_b^0xedu; }
void decode10(S* s,const unsigned int wire[5]){ s->shard=wire[0]^0x65u; s->lease=wire[1]^0x87u; s->rebase=wire[2]^0xa9u; s->lane_a=wire[3]^0xcbu; s->lane_b=wire[4]^0xedu; }
void clear10(A* a){ for(int shard=0; shard<4; ++shard){ a->lease[shard]=0u; a->rebase[shard]=0u; a->lane_a[shard]=0u; a->lane_b[shard]=0u; } }
void absorb10(A* a,const S* s){ unsigned int shard=s->shard%4u; if(s->lease<a->lease[shard]) return; if(s->lease>a->lease[shard]){ a->lease[shard]=s->lease; a->rebase[shard]=s->rebase; a->lane_a[shard]=s->lane_a; a->lane_b[shard]=s->lane_b; return; } if(s->rebase>a->rebase[shard]){ a->rebase[shard]=s->rebase; a->lane_a[shard]=s->lane_a; a->lane_b[shard]=s->lane_b; } else if(s->rebase==a->rebase[shard]){ a->lane_a[shard]+=s->lane_a; a->lane_b[shard]+=s->lane_b; } }
unsigned int sig10(const A* a){ unsigned int h=2166136261u; for(int shard=0; shard<4; ++shard){ h=mix10(h,a->lease[shard]); h=mix10(h,a->rebase[shard]); h=mix10(h,a->lane_a[shard]); h=mix10(h,a->lane_b[shard]); } return h; }
