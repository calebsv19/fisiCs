"""Reviewed control/capability envelope for the bounded successful source path.

Only the listed top-level physical regions are open to abstract interpretation.
The surrounding statements and nine support routines are fixed to reviewed AST
structure. This is an explicit source-envelope restriction, not general CFG or
interprocedural verification. Locations and transient addresses are normalized;
resolved binding relationships, operation order, names and types are preserved.
Never regenerate these pins automatically from a candidate to obtain acceptance.
"""
from identity import digest
from flow_engine import Refusal

OPEN_REGIONS={'cs_motion_initialize':{11:'call',12:'for',13:'for'},'integrate':{11:'for',24:'for',26:'for'}}
REVIEWED={'cs_motion_initialize': '3ebbbfd11be1eeeb97ff7b1b28c95d5b23f7dbbf05950a0d23b9b26a39006f11', 'integrate': '7779b5a6d2a2dd02c0e0663dc1ad8fc633672b75d2b072ef2e797d8dd405ef54', 'work_init': '77536df6373be506e74950b1f23d50a7bc90602f2cbf8be03068631d7b21773e', 'publish': '766893d6ea93654ac4757f0d9be1ec30de84867db4e8128782c405f2ca383c90', 'evaluate': 'a7fc20ec9392d17f701321e6ceb4d3a44d3a953a379d69d084b7cdc7a83bf3d9', 'solve': 'c662a8825f445887673190d397160a46af080784d9fa13e1bcdf873b1d061b2b', 'cs_fiber_system_solve': '47d7b810b2c62b7442c1cee1c49cb549e20ea34757f72c08196f1480c8f4e074', 'correction': '6b7d8d4a2159ae5435ef24a365358fdd0c8ab3258e354bdb733d529fab29d029', 'merit': 'c5318e3367375c6ae51651ed7a9bcb13ca750e9164ab019fc628be70b7c10c98', 'cs_motion_integrate': '73d852a7be068503c7570cf3aba2c1124e7aa36aca9d6401f9491414b1e18aab'}

def envelope(f):
 value=dict(f,body=dict(f['body'],body=list(f['body']['body'])))
 value.pop('return_type',None)
 for index,tag in OPEN_REGIONS.get(f['name'],{}).items():
  body=value['body']['body']
  if index>=len(body) or body[index].get('tag')!=tag:raise Refusal('reviewed source envelope region/order changed: '+f['name'])
  body[index]={'physical_region':index,'expected_tag':tag}
 return value

def verify_envelope(engine):
 checked={}
 for name,expected in REVIEWED.items():
  if name not in engine.functions:raise Refusal('reviewed source envelope routine absent: '+name)
  f=engine.functions[name];engine.last_site=f
  rt=f.get('return_type',{})
  wanted='void' if name in ('work_init','publish') else 'double' if name=='merit' else 'CoreStructuralStatus'
  if (rt.get('name') or rt.get('primitive'))!=wanted or rt.get('pointer') or rt.get('dims') or rt.get('long_double'):raise Refusal('reviewed source envelope return type changed: '+name)
  actual=digest(envelope(f))
  if actual!=expected:raise Refusal('reviewed source envelope changed: '+name)
  checked[name]=actual
 return {'kind':'fixed reviewed AST envelope; selected physical regions interpreted separately','routines':checked,'open_regions':OPEN_REGIONS,'claim':'conditional successful path under documented solver ports; no general CFG proof'}
