from pathlib import Path
import json,subprocess,os,re,hashlib
FUN=['ce_kinetic','ce_momentum_delta','ce_impulse','ce_loss','ce_balance']
def gate(compiler,include,fixture,candidate,out,actual=None):
 out.mkdir();rows=[];selections=[];env={**os.environ,'PYTHONDONTWRITEBYTECODE':'1'};env.pop('FISICS_OVERLAY',None)
 physical=['--overlay=physics-semantics','--physical-semantics-policy',fixture/'policy.json','-DCONTACT_PHYSICAL=1'];inc=['-I'+str(include),'-I'+str(fixture)]
 def run(label,args,ok=True):
  q=subprocess.run(list(map(str,args)),capture_output=True,text=True,env=env,timeout=90);(out/(label+'.log')).write_text(q.stdout+q.stderr);rows.append({'label':label,'argv':list(map(str,args)),'exit':q.returncode,'success_expected':ok});assert (q.returncode==0)==ok,(label,(q.stdout+q.stderr)[-900:]);return q
 for fn in FUN:
  report=out/(fn+'.json');run(fn,[compiler,*physical,*inc,'--physical-semantics-strict','--physical-semantics-allow-literal-assumptions','--physical-semantics-function',fn,'--emit-physical-semantics-json',report,(actual[fn] if actual else fixture/'contracts.c')]);selections.append(json.loads(report.read_text())['selection'])
 run('spin-dimensional',[compiler,'--overlay=physics-units',*inc,'--units-coverage-strict','--units-coverage-allow-literal-assumptions','--units-coverage-function','ce_spin','--emit-units-coverage-json',out/'spin.json',fixture/'contracts.c'])
 for label,param,expr,ok in [('valid','double m CE_M,double v CE_V','ce_kinetic(m,v)',True),('wrong-dimension','double m CE_V,double v CE_V','ce_kinetic(m,v)',False),('same-dimension-wrong-kind','double before CE_D(energy) CE_K("quantity.torque"),double after CE_E','ce_loss(before,after)',False),('raw-restoration','double m,double v','ce_kinetic(m,v)',False)]:
  s=out/(label+'.c');s.write_text('#include "contact_energy.h"\ndouble CE_E check('+param+'){return '+expr+';}\n');run(label,[compiler,*physical,*inc,'--physical-semantics-strict','--physical-semantics-allow-declared-boundaries','--physical-semantics-function','check','--emit-physical-semantics-json',out/(label+'.json'),s],ok)
 # Positive ordinary source and missing required operation ensure model refusals are meaningful.
 policy=json.loads((fixture/'policy.json').read_text());policy['catalog_paths']=[str(fixture/p) for p in policy['catalog_paths']];policy['operation_rules']=[x for x in policy['operation_rules'] if x['id']!='rule.contact.mass.velocity'];mp=out/'missing-rule.json';mp.write_text(json.dumps(policy))
 run('missing-operation',[compiler,'--overlay=physics-semantics','--physical-semantics-policy',mp,'-DCONTACT_PHYSICAL=1',*inc,'--physical-semantics-strict','--physical-semantics-allow-literal-assumptions','--physical-semantics-function','ce_kinetic','--emit-physical-semantics-json',out/'missing-rule-report.json',fixture/'contracts.c'],False)
 # Four portable arithmetic configurations; preprocessing makes ordinary input annotation-free.
 for mode in ['clang','base','units','physical']:
  s=fixture/'runtime.c';flags=[];tool='clang' if mode=='clang' else compiler
  if mode=='base':
   q=run('ordinary-preprocess',['clang','-E','-P','-fno-blocks',*inc,s]);s=out/'ordinary.c';s.write_text(q.stdout)
  if mode=='units':flags=['--overlay=physics-units']
  if mode=='physical':flags=physical
  exe=out/('runtime-'+mode);run(mode+'-compile',[tool,*flags,*inc,s,'-lm','-o',exe]);run(mode+'-run',[exe])
 text=(fixture/'contact_energy.h').read_text()
 for label,old,new in [('half','half CE_R=.5','half CE_R=1'),('momentum-sign','return m*(now-old)','return m*(old-now)'),('impulse-sign','return (closing-target)*effective','return (target-closing)*effective'),('loss-sign','return before-after','return after-before'),('loss-omit','return after-before+loss','return after-before'),('loss-double','return after-before+loss','return after-before+loss+loss')]:
  d=out/label;d.mkdir();(d/'contact_energy.h').write_text(text.replace(old,new));(d/'runtime.c').write_bytes((fixture/'runtime.c').read_bytes());exe=d/'probe';run(label+'-compile',['clang','-I'+str(d),d/'runtime.c','-lm','-o',exe]);run(label+'-detected',[exe],False)
 result={'passed':True,'commands':rows,'physical':selections,'scope':'five actual scalar bodies; ce_spin dimensional only; raw source interfaces not physical-strict'};(out/'results.json').write_text(json.dumps(result,indent=2)+'\n');return result
