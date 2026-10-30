#!/usr/bin/env python3
"""Bounded arithmetic reuse must preserve refusals and source effects."""
import json
from flow_engine import Engine,Refusal,V,ZERO,M

def check():
 e=Engine({});site={'tag':'binary','file':'cache-control.c','line':1,'column':1};checks=0
 def require(value):
  nonlocal checks
  assert value;checks+=1
 def refuses(op,a,b):
  try:e.arithmetic(op,a,b,site)
  except Refusal:return True
  return False
 # Repeated scalar facts reuse results, but different facts must be rechecked.
 require(e.arithmetic('+',4,5,site)==9)
 require(e.arithmetic('+',4,5,site)==9 and e.arithmetic_reuses==1)
 require(refuses('+',4096,1))
 require(refuses('/',4,0))
 require(refuses('-',0,1))
 unary=dict(site,tag='unary')
 require(e.arithmetic('*',-1,2,unary)==-2)
 require(refuses('*',-1,2))
 length=V((0,1,0),deps=frozenset({'length'}))
 require(e.arithmetic('+',length,length,site).dim==(0,1,0))
 require(refuses('+',length,V(M)))
 require(refuses('/',length,V(c=0)))
 # Inactive and live roots are separate complete cache keys.
 for dead in (False,True):
  v=V((0,1,0),deps=frozenset({'live'}),dead=dead)
  result=e.arithmetic('*',v,V(c=2),site)
  require(result.dead==dead and result.deps==(frozenset() if dead else v.deps))
 # Matrix/rate contractions cannot enter the effect-free cache. Every source
 # invocation must still count its own momentum connection and validate roles.
 matrix=V(M,deps=frozenset({'density'}),coeff=(0,0))
 velocity=V((0,1,-1),deps=frozenset({'velocity'}),rate=0)
 before=len(e.rows);size=len(e.arithmetic_cache)
 for line in (10,20):
  value=e.arithmetic('*',matrix,velocity,dict(site,line=line))
  require(value.dim==(1,1,-1) and value.rate==0)
 require(len(e.rows)==before+2 and len(e.arithmetic_cache)==size)
 require(e.momentum_roles=={(0,0)})
 require(refuses('*',matrix,V((0,0,-1),rate=1)))
 # The cache is finite; overflow falls back to the same checked operations.
 for a in range(65):
  for b in range(65):require(e.arithmetic('+',a,b,site)==a+b)
 require(len(e.arithmetic_cache)==4096)
 require(refuses('+',4096,1))
 require(e.arithmetic('+',65,66,site)==131)
 return dict(passed=True,checks=checks,cache_entries=len(e.arithmetic_cache),scope='immutable scalar/index reuse; kind/range/zero/context keys, finite fallback and per-source momentum checks')

if __name__=='__main__':print(json.dumps(check(),indent=2))
