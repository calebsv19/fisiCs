#!/usr/bin/env python3
from pathlib import Path
import subprocess,json,os
import argparse
p=argparse.ArgumentParser()
p.add_argument('--compiler',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
f=Path(__file__).resolve().parents[2];c=a.compiler.resolve();o=a.out.resolve();o.mkdir(parents=True,exist_ok=False);rows=[]
H='#include <fisics/extensions.h>\n#define E FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy")\n'
cases=[('convert-valid','double check(double x FISICS_DIM(length) FISICS_UNIT(foot)) FISICS_DIM(length) FISICS_UNIT(meter){return fisics_convert_unit(x,"meter");}',False,0),('convert-no-source-unit','double check(double x FISICS_DIM(length)) FISICS_DIM(length) FISICS_UNIT(meter){return fisics_convert_unit(x,"meter");}',False,1),('convert-wrong-target','double check(double x FISICS_DIM(length) FISICS_UNIT(foot)) FISICS_DIM(length) FISICS_UNIT(second){return fisics_convert_unit(x,"second");}',False,1),('convert-nonliteral','double check(double x FISICS_DIM(length) FISICS_UNIT(foot),char*p) FISICS_DIM(length) FISICS_UNIT(meter){return fisics_convert_unit(x,p);}',False,1),('direct-valid','E double check(double x E){return *(&x);}',True,0),('raw-direct','E double check(double x){return *(&x);}',True,1),('raw-alias','E double check(double x E){double*p=&x;return *p;}',True,1),('unpaired-address','void check(double x E){&x;}',True,1),('conversion-no-kind-restoration','E double check(double x FISICS_DIM(energy) FISICS_UNIT(joule)){return fisics_convert_unit(x,"joule");}',True,1)]
for n,body,phys,code in cases:
 p=o/(n+'.c');p.write_text(H+body+'\n');report=o/(n+'.json');pre='physical-semantics' if phys else 'units-coverage';args=[c,'-I'+str(f/'include'),'--overlay=physics-semantics','--physical-semantics-policy',f/'tests/fixtures/physical_semantics/policy.json'] if phys else [c,'-I'+str(f/'include'),'--overlay=physics-units'];args+=['--'+pre+'-strict','--'+pre+'-allow-declared-boundaries','--'+pre+'-allow-literal-assumptions','--'+pre+'-function','check','--emit-'+pre+'-json',report,p];q=subprocess.run(list(map(str,args)),cwd=f,env={**os.environ,'FISICS_MAX_PROCS':'0'},capture_output=True,text=True);(o/(n+'.log')).write_text(q.stdout+q.stderr);assert q.returncode==code,(n,q.returncode,q.stderr[-1000:]);d=json.loads(report.read_text());sel=d['selection' if phys else 'strict_policy'];assert sel['accepted']==(code==0);rows.append(dict(id=n,exit=q.returncode,selection=sel))
assert rows[0]['selection']['checked']==3 and rows[0]['selection']['ordinary_controls']==1
assert rows[4]['selection']['checked']==3
(o/'results.json').write_text(json.dumps(dict(passed=True,cases=rows),indent=2));print('Nine repair guards PASS')
