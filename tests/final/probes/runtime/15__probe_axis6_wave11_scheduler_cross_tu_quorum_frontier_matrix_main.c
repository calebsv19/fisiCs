#include <stdio.h>
typedef struct S { unsigned int shard, quorum, frontier, lane_a, lane_b; } S;
typedef struct A { unsigned int quorum[4], frontier[4], lane_a[4], lane_b[4]; } A;
void seed11(S* out,unsigned int shard,unsigned int quorum,unsigned int frontier,unsigned int lane_a,unsigned int lane_b);
void encode11(const S* s,unsigned int wire[5]);
void decode11(S* s,const unsigned int wire[5]);
void clear11(A* a);
void absorb11(A* a,const S* s);
unsigned int sig11(const A* a);
int main(void){ S s0_old,s0_new,s1_old,s1_new,s2_old,s2_new,s3_old,s3_new,d; A c,r; unsigned int w0_old[5],w0_new[5],w1_old[5],w1_new[5],w2_old[5],w2_new[5],w3_old[5],w3_new[5]; seed11(&s0_old,0u,3u,2u,5u,6u); seed11(&s0_new,0u,6u,5u,9u,11u); seed11(&s1_old,1u,2u,1u,4u,7u); seed11(&s1_new,1u,7u,6u,10u,12u); seed11(&s2_old,2u,4u,3u,8u,5u); seed11(&s2_new,2u,8u,7u,14u,15u); seed11(&s3_old,3u,5u,2u,3u,9u); seed11(&s3_new,3u,9u,8u,16u,13u); encode11(&s0_old,w0_old); encode11(&s0_new,w0_new); encode11(&s1_old,w1_old); encode11(&s1_new,w1_new); encode11(&s2_old,w2_old); encode11(&s2_new,w2_new); encode11(&s3_old,w3_old); encode11(&s3_new,w3_new); clear11(&c); absorb11(&c,&s0_new); absorb11(&c,&s1_new); absorb11(&c,&s2_new); absorb11(&c,&s3_new); clear11(&r); decode11(&d,w2_old); absorb11(&r,&d); decode11(&d,w0_new); absorb11(&r,&d); decode11(&d,w3_old); absorb11(&r,&d); decode11(&d,w1_new); absorb11(&r,&d); decode11(&d,w2_new); absorb11(&r,&d); decode11(&d,w0_old); absorb11(&r,&d); decode11(&d,w3_new); absorb11(&r,&d); decode11(&d,w1_old); absorb11(&r,&d); { unsigned int sc=sig11(&c), sr=sig11(&r); printf("%u %u %u\n", sc, sr, (sc==sr)?1u:0u); } return 0; }
