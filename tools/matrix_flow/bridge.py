"""Admitted planar storage path, using original resolved C ASTs."""
from flow_engine import *

def walk(n):
 if isinstance(n,dict):
  if 'tag' in n:yield n
  for k,v in n.items():
   if k not in ['dtype','cast_type']:yield from walk(v)
 elif isinstance(n,list):
  for v in n:yield from walk(v)
def params(f):return [v for p in f['parameters'] for v in p['variables']]
def bind(f,args):
 pp=params(f)
 if len(pp)!=len(args):raise Refusal('port count')
 return Env({p['key']:a for p,a in zip(pp,args)},types={p['key']:p['dtype'] for p in pp})
def readonly_returns(engine,n,env):
 if not n:return []
 t=n['tag']
 if t=='block':
  out=[]
  for s in n['body']:out+=readonly_returns(engine,s,env)
  return out
 if t=='return':return list(indices(engine.expr(n['value'],env)))
 if t=='if':
  condition=engine.expr(n['condition'],env)
  choices=condition.values if isinstance(condition,Idx) else (engine.truth(condition),)
  out=[]
  if any(choices):out+=readonly_returns(engine,n['yes'],Env(env,types=env.types))
  if not all(choices):out+=readonly_returns(engine,n['no'],Env(env,types=env.types))
  return out
 if t=='for':
  engine.stmt(n['init'],env);out=[];count=0
  while engine.truth(engine.expr(n['condition'],env)):
   if count>27:raise Refusal('map loop limit')
   count+=1;out+=readonly_returns(engine,n['body'],env);engine.expr(n['increment'],env)
  return out
 if t=='decl':engine.stmt(n,env);return []
 raise Refusal('unmodelled map effect '+t)
def verify_rotation(engine):
 f=engine.functions['cs_pf_rotation'];p=params(f)
 if f.get('return_type',{}).get('name')!='size_t' or f['return_type'].get('pointer'):raise Refusal('rotation map return type changed')
 if len(p)!=3 or p[0]['dtype']['name']!='CoreStructuralPlasticFrameModel':raise Refusal('map parameter identity/type')
 for n in walk(f['body']):
  if n['tag'] in ['assign','call','cast','goto','unsupported']:raise Refusal('unsupported map side effect')
  if n['tag']=='unary' and n['op'] in ['++','--'] and (n['left']['tag']!='id' or n['left']['origin'] in {v['key'] for v in p}):raise Refusal('map parameter mutation')
 cases=0;union=set();engine.rotation_summaries={}
 for nodes in range(2,10):
  for joints in range(10-nodes):
   for end in range(2):
    member=Record('map.member',dict(node_a=Idx(tuple(range(nodes))),node_b=Idx(tuple(range(nodes)))))
    mm=Array((8,),'model.member',uniform=lambda ix:member)
    joint=Record('map.joint',dict(member=Idx(tuple(range(8))),end=Idx((0,1))))
    jj=Array((joints,),'model.active-joints',uniform=lambda ix:joint)
    m=Record('map.model',dict(nodes=nodes,joints=joints,member=mm,joint=jj))
    values=readonly_returns(engine,f['body'],bind(f,[m,Idx(tuple(range(8))),end]))
    if not values or any(i<0 or i>=3*(nodes+joints) or i%3!=2 for i in values):raise Refusal('map role/active-bound contradiction')
    engine.rotation_summaries[(nodes,joints,end)]=Idx(tuple(sorted(set(values))));union.update(values);cases+=1
 engine.rotation_summary=Idx(tuple(sorted(union)));return cases

def vector(name,dim,rate=False):
 return Array((24,),name,layout=lambda ix:(dim(role(ix[0])),None,role(ix[0]) if rate else None),uniform=lambda ix:V(dim(role(ix[0])),deps=frozenset([name]),rate=role(ix[0]) if rate else None))
def displacement(name):return vector(name,lambda r:(0,1-r,0),True)
def state(name):
 return Record(name,dict(frame=Record(name+'.frame',dict(state=Record(name+'.frame.state',dict(displacement=displacement(name+'.displacement'),applied=force(name+'.applied'))))),velocity=vector(name+'.velocity',lambda r:(0,1-r,-1),True),acceleration=vector(name+'.acceleration',lambda r:(0,1-r,-2),True),inertial=Array((24,),name+'.inertial',lambda ix:((1,1+role(ix[0]),-2),None,role(ix[0]))),damping=Array((24,),name+'.damping',lambda ix:((1,1+role(ix[0]),-2),None,role(ix[0])))))
def force(name):return vector(name,lambda r:(1,1+r,-2),True)
def make_model():
 area=Array((16,),'member.fiber_area',uniform=lambda ix:V((0,2,0),deps=frozenset(['section.area'])))
 member=Record('member',dict(node_a=Idx(tuple(range(8))),node_b=Idx(tuple(range(8))),fiber_area_m2=area))
 return Record('model',dict(nodes=8,joints=0,members=7,fibers=16,corotational=False,member=Array((7,),'model.member',uniform=lambda ix:member),x_m=Array((8,),'model.x',uniform=lambda ix:V(L,deps=frozenset(['geometry.x']))),y_m=Array((8,),'model.y',uniform=lambda ix:V(L,deps=frozenset(['geometry.y']))),fixed=Array((24,),'model.fixed',uniform=lambda ix:ix[0]<3)))
def owner(m,mass):
 settings=Record('settings',dict(density_kg_m3=Array((8,),'settings.density',uniform=lambda ix:V((1,-3,0),deps=frozenset(['density']))),added_mass_kg=Array((8,),'settings.added_mass',uniform=lambda ix:V(M,deps=frozenset(['added_mass']))),alpha_m_per_s=V((0,0,-1),deps=frozenset(['decay'])),dt_s=V(TIME,deps=frozenset(['time_step']))))
 return Record('owner',dict(model=Record('original-model',dict(failed_member=3)),active=Record('active',dict(model=m)),settings=settings,mass=mass,direct_frame=False,accepted=state('accepted')))
def call_name(n,name):return n and n.get('tag')=='call' and n['callee'].get('tag')=='id' and n['callee'].get('direct_function') and n['callee'].get('name')==name
def contains(n,predicate):return any(predicate(x) for x in walk(n))
def field(n,name):return n and n.get('tag')=='field' and n.get('field')==name

def verify_path(engine):
 from envelope import verify_envelope
 reviewed_envelope=verify_envelope(engine)
 for name in ('cs_motion_initialize','integrate'):
  first=params(engine.functions[name])[0]['dtype']
  if first['name']!='CoreStructuralFrameMotion' or first['pointer']!=1 or first['const'] or first['volatile']:raise Refusal('persistent owner pointer identity/type contract changed')
 rotation_cases=verify_rotation(engine)
 # Full actual constructor and transform, not declared result trust.
 element=Record('element',dict(length=V(L,deps=frozenset(['geometry.length'])),cosine=V(deps=frozenset(['basis.cosine'])),sine=V(deps=frozenset(['basis.sine']))))
 layout=lambda ix:(coefficient(ix[0]//6,ix[0]%6),(role(ix[0]//6),role(ix[0]%6)))
 output=Array((36,),'producer.output',layout)
 engine.call('cd_consistent_mass',[element,V(M,deps=frozenset(['mass'])),output])
 if len(output.data)!=36:raise Refusal('producer output coverage')
 witnesses=set(v.coeff for v in output.data.values() if isinstance(v,V) and not v.inactive and 'mass' in v.deps)
 if witnesses!={(0,0),(0,1),(1,0),(1,1)}:raise Refusal('producer nonvacuity across four blocks')
 # Actual already-reduced non-corotational owner, seven of eight members.
 f=engine.functions['cs_motion_initialize'];m=make_model();mass=Array((27,27),'owner.mass',lambda ix:(coefficient(*ix),(role(ix[0]),role(ix[1]))));o=owner(m,mass);env=bind(f,[o,None,None,None])
 engine.active_dofs=3*(m.get('nodes')+m.get('joints'))
 body=f['body']['body'];starts=[i for i,n in enumerate(body) if call_name(n,'memset') and contains(n,lambda x:field(x,'mass'))]
 if len(starts)!=1:raise Refusal('unique matrix initialization region absent')
 start=starts[0];zero=body[start]
 if len(zero['args'])!=3 or not field(zero['args'][0],'mass') or zero['args'][1].get('value')!='0' or zero['args'][2]['tag']!='sizeof':raise Refusal('zero-layout contract shape')
 # Source shape binds 27x27 double storage; sizeof must resolve the same array.
 if zero['args'][0]['dtype']['dims']!=[27,27] or zero['args'][0]['dtype']['primitive']!='double':raise Refusal('owner storage ABI/extent mismatch')
 if engine.expr(zero['args'][0],env) is not engine.expr(zero['args'][2]['value'],env):raise Refusal('partial/wrong buffer zeroing')
 for ix in product(range(27),repeat=2):mass.write(ix,0,engine,zero)
 mass.active_bound=engine.active_dofs
 # Replay only source declarations needed by the region; do not bypass unknown writes.
 for s in body[:start]:
  if s['tag']=='decl':
   for v in s['variables']:
    if v['dtype']['name']=='CoreStructuralPlasticFrameModel' or v['name']=='n':engine.stmt(dict(s,variables=[v]),env)
    if v['name']=='n' and env[v['key']]!=engine.active_dofs:raise Refusal('initializer active DOF count changed')
 regions=body[start+1:start+3]
 if len(regions)!=2 or any(x['tag']!='for' for x in regions):raise Refusal('assembly/added-mass region shape')
 engine.stmt(regions[0],env);engine.stmt(regions[1],env)
 if len(mass.data)!=729:raise Refusal('owner storage coverage')
 # Bridge from this exact persistent field into the actual later consumer loop.
 g=engine.functions['integrate'];genv=bind(g,[o,None,None,None,None]);work=Record('work',dict(result=Record('work.result',dict(displacement=displacement('solved.displacement'),internal=force('internal'),reaction=force('reaction'),residual_n=V((1,1,-2),rate=0),residual_nm=V((1,2,-2),rate=1)))));target=Record('target',dict(load=force('applied')))
 predictor=None
 # Evaluate source expressions for every aliased/count/time/accumulator local.
 # Allocated work and target are separate, explicitly admitted solver output ports.
 bindings={};energy=[]
 for statement in g['body']['body']:
  engine.last_site=statement
  if statement['tag']!='decl':continue
  for v in statement['variables']:
   tp=v['dtype']['name'];name=v['name'];value=None
   if tp in ('CoreStructuralPlasticFrameModel','CoreStructuralFrameMotionState') or name in ('n','dt'):
    value=engine.expr(v['init'],genv)
    if tp=='CoreStructuralPlasticFrameModel' and value is not m:raise Refusal('actual model identity changed')
    if tp=='CoreStructuralFrameMotionState':
     if v['dtype']['pointer'] or v['dtype']['dims'] or v['dtype']['volatile']:raise Refusal('old state is not an independent by-value copy')
     if value is not o.get('accepted'):raise Refusal('actual old state identity changed')
     # Copy scalar field containers: C struct-value copy is not an alias of accepted.
     value=state('old')
    if name=='n' and value!=3*(m.get('nodes')+m.get('joints')):raise Refusal('actual active DOF count changed')
    if name=='dt' and value is not o.get('settings').get('dt_s'):raise Refusal('actual time-step identity changed')
   elif tp=='CsFiberWork':
    if not call_name(v['init'],'core_calloc'):raise Refusal('work storage allocation binding changed')
    value=work
   elif tp in ('CoreStructuralPlasticFrameTarget','CoreStructuralFiberBeamTarget'):
    if v['init']['tag']!='aggregate':raise Refusal('target local binding changed')
    value=target
   elif tp=='DynamicTrial':
    init=v['init']
    if init['tag']!='aggregate' or len(init['entries'])!=1 or init['entries'][0]['field']!='owner' or engine.expr(init['entries'][0]['value'],genv) is not o:raise Refusal('actual predictor owner binding changed')
    predictor=Record('predictor',dict(owner=o,predictor=Array((24,),'predictor.displacement',lambda ix:((0,1-role(ix[0]),0),None,role(ix[0]))),velocity_predictor=Array((24,),'predictor.velocity',lambda ix:((0,1-role(ix[0]),-1),None,role(ix[0])))))
    value=predictor
   elif name in ('kinetic','damping','external'):
    actual=engine.expr(v['init'],genv)
    if val(actual).dim!=ZERO or val(actual).c!=0:raise Refusal('energy accumulator zero contract changed')
    value=V(ENERGY,0);energy.append(name)
   if value is not None:genv[v['key']]=value;genv.types[v['key']]=v['dtype'];bindings[name]=v
 if set(energy)!={'kinetic','damping','external'}:raise Refusal('energy accumulator initialization absent')
 loops=[n for n in g['body']['body'] if n['tag']=='for' and contains(n,lambda x:call_name(x,'cs_de_kinetic'))]
 if len(loops)!=1:raise Refusal('unique actual consumer loop absent')
 consumer=loops[0]
 predictors=[n for n in g['body']['body'] if n['tag']=='for' and contains(n,lambda x:x['tag']=='assign' and field(x['target'].get('base'),'predictor'))]
 kinematics=[n for n in g['body']['body'] if n['tag']=='for' and contains(n,lambda x:call_name(x,'cs_de_work'))]
 if len(predictors)!=1 or len(kinematics)!=1 or predictor is None:raise Refusal('actual kinematic producer region absent')
 actual=g['body']['body']
 if actual[11] is not predictors[0] or actual[24] is not kinematics[0] or actual[26] is not consumer:raise Refusal('selected physical region execution order changed')
 engine.stmt(predictors[0],genv)
 # Solver result displacement is an explicit port, after the predictor writes.
 work.get('result').fields['displacement']=displacement('solved.displacement')
 engine.stmt(kinematics[0],genv)
 engine.stmt(consumer,genv)
 consumer_lineage={}
 for name in ('kinetic','damping'):
  value=val(genv[bindings[name]['key']]);required={'density','solved.displacement'} | ({'decay','time_step','old.velocity'} if name=='damping' else set())
  if value.dim!=ENERGY or value.rate is not None or value.inactive or not required.issubset(value.deps):raise Refusal('actual '+name+' source/root connection absent')
  consumer_lineage[name]=sorted(value.deps)
 from effects import audit_effects,verify_null_cancel
 null_cancel=verify_null_cancel(engine)
 effects=audit_effects(engine,[zero,*regions],[predictors[0],kinematics[0],consumer])
 if engine.momentum_roles!={(0,0),(0,1),(1,0),(1,1)}:raise Refusal('four-block actual momentum contraction witness absent')
 return dict(reviewed_envelope=reviewed_envelope,consumer_lineage=consumer_lineage,momentum_roles=sorted(engine.momentum_roles),null_cancel=null_cancel,effects=effects,rotation_cases=rotation_cases,producer_cells=36,owner_cells=729,active_dofs=engine.active_dofs,active_members=7,original_members=8,failed_member=3,direct_frame=False,producer_role_witnesses=sorted(witnesses),regions=[dict(file=n['file'],line=n['line'],column=n['column']) for n in [zero,*regions,consumer]],mass=mass,owner=o)
