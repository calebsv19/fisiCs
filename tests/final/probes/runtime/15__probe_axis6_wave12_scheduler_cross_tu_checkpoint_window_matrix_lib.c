typedef struct S { unsigned int shard, checkpoint, window, lane_a, lane_b; } S;
typedef struct A { unsigned int checkpoint[4], window[4], lane_a[4], lane_b[4]; } A;
static unsigned int mix12(unsigned int h,unsigned int v){ h ^= v + 0x9e3779b9u + (h<<6) + (h>>2); return h; }
void seed12(S* out,unsigned int shard,unsigned int checkpoint,unsigned int window,unsigned int lane_a,unsigned int lane_b){ out->shard=shard%4u; out->checkpoint=checkpoint; out->window=window; out->lane_a=lane_a; out->lane_b=lane_b; }
void encode12(const S* s,unsigned int wire[5]){ wire[0]=s->shard^0x85u; wire[1]=s->checkpoint^0xa7u; wire[2]=s->window^0xc9u; wire[3]=s->lane_a^0xebu; wire[4]=s->lane_b^0x1du; }
void decode12(S* s,const unsigned int wire[5]){ s->shard=wire[0]^0x85u; s->checkpoint=wire[1]^0xa7u; s->window=wire[2]^0xc9u; s->lane_a=wire[3]^0xebu; s->lane_b=wire[4]^0x1du; }
void clear12(A* a){ for(int shard=0; shard<4; ++shard){ a->checkpoint[shard]=0u; a->window[shard]=0u; a->lane_a[shard]=0u; a->lane_b[shard]=0u; } }
void absorb12(A* a,const S* s){ unsigned int shard=s->shard%4u; if(s->checkpoint<a->checkpoint[shard]) return; if(s->checkpoint>a->checkpoint[shard]){ a->checkpoint[shard]=s->checkpoint; a->window[shard]=s->window; a->lane_a[shard]=s->lane_a; a->lane_b[shard]=s->lane_b; return; } if(s->window>a->window[shard]){ a->window[shard]=s->window; a->lane_a[shard]=s->lane_a; a->lane_b[shard]=s->lane_b; } else if(s->window==a->window[shard]){ a->lane_a[shard]+=s->lane_a; a->lane_b[shard]+=s->lane_b; } }
unsigned int sig12(const A* a){ unsigned int h=2166136261u; for(int shard=0; shard<4; ++shard){ h=mix12(h,a->checkpoint[shard]); h=mix12(h,a->window[shard]); h=mix12(h,a->lane_a[shard]); h=mix12(h,a->lane_b[shard]); } return h; }
