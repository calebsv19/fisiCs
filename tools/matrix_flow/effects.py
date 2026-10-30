"""Fail-closed storage-capability audit for the selected actual owner routines.

Disjoint owner subobjects are capabilities under the explicit live/exclusive C
object contract. This is not general alias analysis or a whole-program theorem.
"""
from bridge import walk,params,contains,field,call_name
from flow_engine import Refusal

def tree(n,parent=None,edge=None):
 if isinstance(n,dict):
  if 'tag' in n:yield n,parent,edge;parent=n
  for k,v in n.items():
   if k not in ('dtype','cast_type'):yield from tree(v,parent,k)
 elif isinstance(n,list):
  for v in n:yield from tree(v,parent,edge)
def root(n):
 while n and n.get('tag') in ('field','index','unary'):
  if n['tag']=='unary' and n['op']!='&':break
  n=n['base'] if n['tag'] in ('field','index') else n['left']
 return n

def audit_effects(engine,selected,state_regions):
 fs=engine.functions;count=0
 required=['cs_motion_initialize','integrate','work_init','publish','evaluate','solve','cs_fiber_system_solve','correction','merit']
 for name in required:
  if name not in fs:raise Refusal('effect implementation unavailable: '+name)
 selected_ids={n['id'] for r in selected for n in walk(r)}
 state_ids={n['id'] for r in state_regions for n in walk(r)}
 # Every occurrence is checked, including paths outside selected physical loops.
 for name in required:
  f=fs[name];pp=params(f);nodes=list(tree(f['body']));parent={n['id']:(p,e) for n,p,e in nodes}
  immutable_locals={v['key'] for n,_,_ in nodes if n['tag']=='decl' for v in n['variables'] if v['dtype']['name'] in ('CoreStructuralPlasticFrameModel','CoreStructuralFrameMotionState') or (name in ('integrate','cs_motion_initialize') and v['name'] in ('n','dt'))}
  owners={v['key'] for p in f['parameters'] for v in p.get('variables',[]) if v['dtype']['name']=='CoreStructuralFrameMotion'}
  trials=set();systems=set()
  for n,_,_ in nodes:
   if n['tag']=='decl':
    for v in n['variables']:
     tp=v['dtype']['name']
     if tp=='CoreStructuralFrameMotion':
      if name!='evaluate' or not field(v['init'],'owner') or v['init']['base'].get('origin') not in trials or v['dtype']['pointer']!=1:raise Refusal('unproved owner alias declaration')
      owners.add(v['key'])
     if tp=='DynamicTrial':
      if name=='evaluate' and (v['dtype']['pointer']!=1 or not v['dtype']['const'] or v['init'].get('tag')!='id' or v['init'].get('origin')!=pp[0]['key']):raise Refusal('callback receiver is not the actual solver argument')
      trials.add(v['key'])
     if tp=='CsFiberSystem':systems.add(v['key'])
  work_keys={v['key'] for n,_,_ in nodes if n['tag']=='decl' for v in n['variables'] if v['dtype']['name']=='CsFiberWork'} if name=='integrate' else set()
  pp=params(f);formal_keys={v['key'] for v in pp}
  targets={v['key'] for n,_,_ in nodes if n['tag']=='decl' for v in n['variables'] if v['dtype']['name'] in ('CoreStructuralPlasticFrameTarget','CoreStructuralFiberBeamTarget')} if name=='integrate' else set()
  if name=='evaluate':trials.add(pp[0]['key']) # void receiver, only typed local alias below
  if name in ('cs_fiber_system_solve','correction','merit'):systems.add(pp[0]['key'])
  matrix=pp[1]['key'] if name=='solve' else None
  for n,p,e in nodes:
   engine.last_site=n;t=n['tag']
   if t=='call':
    callee=n['callee']
    native_constant=callee['tag']=='id' and callee.get('name') in ('__builtin_huge_val','__builtin_huge_valf','__builtin_huge_vall') and not n['args'] and (callee.get('origin')=='0' or callee.get('origin','').endswith('#0')) and not callee['dtype']['pointer']
    if callee['tag']=='id' and not callee.get('direct_function') and not native_constant:raise Refusal('indirect/unresolved effect call '+name+':'+callee.get('name','?'))
    if callee['tag']!='id' and not(name=='cs_fiber_system_solve' and field(callee,'evaluate') and callee['base'].get('origin') in systems):raise Refusal('unbound effect callback')
   if t in ('unsupported','cast'):
    if t=='unsupported' or (n['cast_type']['pointer'] and not(n['value']['tag']=='number' and n['value']['value']=='0')):raise Refusal('unmodelled effect/cast in '+name)
   if t=='id' and n['origin'] in owners:
    count+=1
    if p and p['tag']=='field' and e=='base':continue
    if p and p['tag']=='call' and e=='args' and p['args'][0] is n and any(call_name(p,x) for x in ('work_init','publish')):continue
    # The sole permitted owner-bearing callback receiver is checked below.
    if name=='integrate' and p and p['tag']=='aggregate':
     if any(x['field']=='owner' and x['value'] is n for x in p['entries']):continue
    raise Refusal('owner address escape/replacement in '+name)
   if t=='field' and n['field']=='mass':
    base=n['base']
    if base['tag']!='id' or base['origin'] not in owners:raise Refusal('mass field owner identity lost')
    x=n;q,edge=parent[x['id']];rank=0
    while q and q['tag']=='index' and edge=='base':
     rank+=1;x=q;q,edge=parent[x['id']]
    count+=1
    if q and q['tag']=='sizeof':continue
    if q and q['tag']=='call':
     if call_name(q,'memset') and q['id'] in selected_ids and q['args'][0] is n:continue
     if call_name(q,'solve') and q['args'][1] is n:continue
     if rank<2:raise Refusal('matrix address escaped to callee')
    if rank!=2:raise Refusal('partial/untracked matrix address use')
    if q and ((q['tag']=='assign' and edge=='target') or (q['tag']=='unary' and q['op'] in ('++','--','&'))):
     if q['id'] not in selected_ids:raise Refusal('unselected late matrix write/address escape')
   if t=='id' and n['origin'] in work_keys:
    if p and p['tag']=='field' and e=='base':pass
    elif p and p['tag']=='call' and any(call_name(p,x) for x in ('work_init','cs_fiber_system_solve','cs_plastic_frame_evaluate','publish','core_free')):pass
    elif p and p['tag']=='unary' and p['op']=='!':pass
    elif p and p['tag']=='unary' and p['op']=='*' and parent[p['id']][0]['tag']=='sizeof':pass
    else:raise Refusal('unknown solver-work address/alias escape '+str(n['line'])+':'+str(n['column'])+' '+str(p and p['tag']))
   if t=='assign' or (t=='unary' and n['op'] in ('++','--','&')):
    target=n['target'] if t=='assign' else n['left'];r=root(target)
    if r and r.get('tag')=='id' and r['origin'] in formal_keys and target['tag']=='id':raise Refusal('selected path formal/callback binding modified or escaped')
    if r and r.get('tag')=='id' and r['origin'] in owners and contains(target,lambda x:field(x,'settings') or field(x,'model') or field(x,'direct_frame')):
     allowed=False
     if t=='unary' and n['op']=='&' and field(target,'model'):
      q,_=parent[n['id']]
      if q and q['tag']=='decl' and any(v['init'] is n and v['dtype']['name']=='CoreStructuralPlasticFrameModel' and v['dtype']['const'] and v['dtype']['pointer']==1 for v in q['variables']):allowed=True
      if q and q['tag']=='call' and any(call_name(q,x) for x in ('cs_frame_stability','cs_pf_joint_result','cs_rf_expand')):allowed=True
     if not allowed:raise Refusal('admitted owner/model settings modified or escaped')
    if r and r.get('tag')=='id' and r['origin'] in targets and not(t=='unary' and n['op']=='&' and target['tag']=='id'):raise Refusal('target force port modified or escaped')
    if r and r.get('tag')=='id' and r['origin'] in immutable_locals and not(t=='unary' and n['op']=='&' and n['id'] in selected_ids and target['tag']!='id'):raise Refusal('immutable local/model/state alias modified or escaped '+name+':'+str(n['line'])+':'+str(n['column']))
    if name=='integrate' and r and r.get('tag')=='id' and r['origin'] in work_keys and contains(target,lambda x:field(x,'displacement') or field(x,'internal') or field(x,'reaction')) and n['id'] not in state_ids:raise Refusal('unselected physical work-state write/address escape')
    if name=='integrate' and r and r.get('tag')=='id' and r['origin'] in owners and contains(target,lambda x:field(x,'accepted')):
     if contains(target,lambda x:field(x,'velocity') or field(x,'acceleration')) and n['id'] not in state_ids:raise Refusal('unselected current-state write/address escape')
     if field(target,'accepted'):raise Refusal('whole accepted-state replacement')
   if t=='assign':
    r=root(n['target'])
    if r and r.get('tag')=='id' and r['origin'] in owners:
     target=n['target']
     if target['tag']=='field' and target['base'] is r and target['field'] in ('active','settings','model','direct_frame'):raise Refusal('owner/model capability replaced')
   if t=='id' and n['origin'] in targets:
    if p and p['tag']=='field' and e=='base':pass
    elif p and p['tag']=='unary' and p['op']=='&':
     q,_=parent[p['id']]
     if not(q and q['tag']=='call' and any(call_name(q,x) for x in ('work_init','cs_fiber_system_solve','cs_plastic_frame_evaluate'))):raise Refusal('target capability escaped')
    else:raise Refusal('target capability aliased/replaced')
   if t=='field' and ((n['field']=='load' and root(n).get('origin') in targets) or (n['field'] in ('displacement','internal','reaction') and root(n).get('origin') in work_keys)):
    x=n;q,edge=parent[x['id']];rank=0
    while q and q['tag']=='index' and edge=='base':rank+=1;x=q;q,edge=parent[x['id']]
    if rank==1:pass
    elif q and q['tag']=='sizeof':pass
    elif n['field']=='load' and call_name(q,'memcpy') and q['args'][0] is n:
     ancestor=q;guard=None
     while ancestor and ancestor['id'] in parent:
      ancestor,_=parent[ancestor['id']]
      if ancestor and ancestor['tag']=='if':guard=ancestor;break
     if not guard or guard['condition'].get('origin')!=pp[1]['key'] or q['args'][1].get('origin')!=pp[1]['key']:raise Refusal('unforced endpoint null guard changed')
    else:raise Refusal('physical work/target array address escaped')
   if t=='id' and matrix and n['origin']==matrix:
    count+=1;x=n;q,edge=parent[x['id']];rank=0
    while q and q['tag']=='index' and edge=='base':rank+=1;x=q;q,edge=parent[x['id']]
    if rank!=2 or (q and ((q['tag']=='assign' and edge=='target') or q['tag'] in ('unary','cast'))):raise Refusal('solve matrix is not read-only')
   if t=='id' and n['origin'] in trials:
    count+=1
    if p and p['tag']=='field' and e=='base':continue
    if name=='evaluate' and p and p['tag']=='decl':continue # const DynamicTrial *v=ptr, checked receiver field-only
    if name=='integrate' and p and p['tag']=='unary' and p['op']=='&':
     q,_=parent[p['id']]
     if q and q['tag']=='aggregate' and any(x['field']=='model' and x['value'] is p for x in q['entries']):continue
    raise Refusal('callback receiver address escape')
   if t=='id' and n['origin'] in systems:
    count+=1
    if p and p['tag']=='field' and e=='base':continue
    if p and p['tag']=='call' and p['args'][0] is n and any(call_name(p,x) for x in ('correction','merit')):continue
    if name=='integrate' and p and p['tag']=='unary' and p['op']=='&':
     q,_=parent[p['id']]
     if call_name(q,'cs_fiber_system_solve') and q['args'][0] is p:continue
    raise Refusal('solver capability escaped/replaced')
   if t=='field' and n['field'] in ('model','evaluate') and n['base']['tag']=='id' and n['base']['origin'] in systems:
    count+=1
    if n['field']=='evaluate':
     if not (p and p['tag']=='call' and e=='callee' and field(p['args'][0],'model') and p['args'][0]['base']['origin']==n['base']['origin']):raise Refusal('unbound solver callback')
    elif not(p and p['tag']=='call' and p['args'][0] is n and field(p['callee'],'evaluate') and p['callee']['base']['origin']==n['base']['origin']):raise Refusal('opaque model capability escaped')
  # Every cancellation edge forwards the admitted null formal; no name-only waiver.
  if name in ('cs_motion_initialize','integrate','cs_fiber_system_solve'):
   cancellation=pp[1 if name=='cs_motion_initialize' else 2 if name=='integrate' else 4]['key']
   for n,_,_ in nodes:
    if call_name(n,'cs_fiber_cancel'):
     if n['args'][0].get('origin')!=cancellation:raise Refusal('null cancellation binding changed')
    if call_name(n,'cs_fiber_system_solve') and n['args'][4].get('origin')!=cancellation:raise Refusal('solver cancellation forwarding changed')
  # Pointer-bearing aggregate initializers are checked by field and direct binding.
  if name=='integrate':
   aggregates=[v for n,_,_ in nodes if n['tag']=='decl' for v in n['variables'] if v['dtype']['name'] in ('DynamicTrial','CsFiberSystem')]
   if len(aggregates)!=2:raise Refusal('callback capability construction changed')
   bytype={v['dtype']['name']:v for v in aggregates};v=bytype['DynamicTrial'];s=bytype['CsFiberSystem'];ve=v['init'];se=s['init']
   if ve['tag']!='aggregate' or len(ve['entries'])!=1 or ve['entries'][0]['field']!='owner' or ve['entries'][0]['value']['origin'] not in owners:raise Refusal('callback owner binding changed')
   entries={e['field']:e['value'] for e in se['entries']}
   if set(entries)!={'dofs','fixed','force_scale','moment_scale','energy_line_search','model','evaluate'}:raise Refusal('callback system fields changed')
   if entries['evaluate']['tag']!='id' or not entries['evaluate']['direct_function'] or entries['evaluate']['name']!='evaluate':raise Refusal('callback target changed')
   model=entries['model']
   if model['tag']!='unary' or model['op']!='&' or model['left'].get('origin')!=v['key']:raise Refusal('callback receiver changed')
 return dict(capability_uses=count,routines=required,contract='exclusive live owner; disjoint valid C subobjects; null cancellation; no intervening external owner operation')

def verify_null_cancel(engine):
 f=engine.functions.get('cs_fiber_cancel')
 if not f:raise Refusal('cancellation implementation unavailable')
 pp=params(f);body=f['body']['body']
 if len(pp)!=3 or len(body)!=2 or body[0]['tag']!='if' or body[1]['tag']!='return' or body[1]['value'].get('value')!='0':raise Refusal('null cancellation continuation changed')
 c=body[0]['condition']
 if c['tag']!='binary' or c['op']!='&&' or c['left'].get('origin')!=pp[0]['key']:raise Refusal('null cancellation guard changed')
 rhs=c['right']
 if rhs['tag']!='call' or rhs['callee'].get('origin')!=pp[0]['key'] or len(rhs['args'])!=1 or rhs['args'][0].get('origin')!=pp[1]['key'] or body[0]['no'] is not None:raise Refusal('null cancellation callback edge changed')
 return 'source-checked C short-circuit; null callback takes return false'
