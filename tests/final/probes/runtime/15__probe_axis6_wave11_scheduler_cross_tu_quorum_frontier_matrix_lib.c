typedef struct S { unsigned int shard, quorum, frontier, lane_a, lane_b; } S;
typedef struct A { unsigned int quorum[4], frontier[4], lane_a[4], lane_b[4]; } A;
static unsigned int mix11(unsigned int h,unsigned int v){ h ^= v + 0x9e3779b9u + (h<<6) + (h>>2); return h; }
void seed11(S* out,unsigned int shard,unsigned int quorum,unsigned int frontier,unsigned int lane_a,unsigned int lane_b){ out->shard=shard%4u; out->quorum=quorum; out->frontier=frontier; out->lane_a=lane_a; out->lane_b=lane_b; }
void encode11(const S* s,unsigned int wire[5]){ wire[0]=s->shard^0x75u; wire[1]=s->quorum^0x97u; wire[2]=s->frontier^0xb9u; wire[3]=s->lane_a^0xdbu; wire[4]=s->lane_b^0xfdu; }
void decode11(S* s,const unsigned int wire[5]){ s->shard=wire[0]^0x75u; s->quorum=wire[1]^0x97u; s->frontier=wire[2]^0xb9u; s->lane_a=wire[3]^0xdbu; s->lane_b=wire[4]^0xfdu; }
void clear11(A* a){ for(int shard=0; shard<4; ++shard){ a->quorum[shard]=0u; a->frontier[shard]=0u; a->lane_a[shard]=0u; a->lane_b[shard]=0u; } }
void absorb11(A* a,const S* s){ unsigned int shard=s->shard%4u; if(s->quorum<a->quorum[shard]) return; if(s->quorum>a->quorum[shard]){ a->quorum[shard]=s->quorum; a->frontier[shard]=s->frontier; a->lane_a[shard]=s->lane_a; a->lane_b[shard]=s->lane_b; return; } if(s->frontier>a->frontier[shard]){ a->frontier[shard]=s->frontier; a->lane_a[shard]=s->lane_a; a->lane_b[shard]=s->lane_b; } else if(s->frontier==a->frontier[shard]){ a->lane_a[shard]+=s->lane_a; a->lane_b[shard]+=s->lane_b; } }
unsigned int sig11(const A* a){ unsigned int h=2166136261u; for(int shard=0; shard<4; ++shard){ h=mix11(h,a->quorum[shard]); h=mix11(h,a->frontier[shard]); h=mix11(h,a->lane_a[shard]); h=mix11(h,a->lane_b[shard]); } return h; }
