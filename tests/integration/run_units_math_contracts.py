#!/usr/bin/env python3
"""Physical math-helper diagnostics, runtime and explicit unchecked controls."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
def check(compiler,out):
    manifest=json.loads((ROOT/'tests/final/meta/15-torture-differential-physics-units-math-helpers.json').read_text())
    codes=[None,4103,4116,4116,4107,4107,4116,4116,4112,4113,4107,4110,None,4116,4116,4107,4107,4116]
    assert len(manifest['tests'])==len(codes)
    rows=[]
    env={**os.environ,'FISICS_MAX_PROCS':'0'}
    for case,code in zip(manifest['tests'],codes):
        name=case['id'];exe=out/name;diag=out/(name+'.json')
        cmd=[str(compiler),*case['args'],'--emit-diags-json',str(diag),str(ROOT/'tests/final'/case['input']),'-o',str(exe)]
        p=subprocess.run(cmd,cwd=ROOT,env=env,text=True,capture_output=True,timeout=60)
        (out/(name+'.log')).write_text(p.stdout+p.stderr)
        assert p.returncode==int(code is not None),(name,p.returncode,p.stdout,p.stderr)
        if code is None:
            q=subprocess.run([str(exe)],text=True,capture_output=True,timeout=10)
            assert q.returncode==0 and q.stdout==(ROOT/'tests/final/expect/15__units_math_helpers_valid.stdout').read_text(),(name,q)
            exe.unlink()
        else:
            assert not exe.exists(),name
            errors=[d for d in json.loads(diag.read_text())['diagnostics'] if d['severity_name']=='error']
            assert len(errors)==1 and errors[0]['code_id']==code,(name,errors)
            d=errors[0];assert d['line']>0 and d['column']>0 and d['category_name']=='extension',d
            if code==4116:
                details=d['details'];op='hypot' if 'hypot' in name else 'fmax'
                assert details['context']==op,details
                length=[1,0,0,0,0,0,0,0];other=[0,0,0,0,0,0,0,0] if 'zero' in name else [0,0,1,0,0,0,0,0]
                assert details['lhs_dim']==(other if 'hypot_zero' in name else length),details
                assert details['rhs_dim']==(length if 'hypot_zero' in name else other),details
            if code==4107 and ('hypot_units' in name or 'fmax_units' in name):
                assert d['details']['context']==('hypot' if 'hypot' in name else 'fmax'),d
                assert d['details']['source_unit']['name']=='foot' and d['details']['target_unit']['name']=='meter',d
        rows.append(dict(id=name,command=cmd,exit=p.returncode,expected_code=code))
    for op in ['fabs','hypot','fmax']:
        name=op+'_unchecked';src=out/(name+'.c');obj=out/(name+'.o')
        src.write_text('#include <math.h>\nvoid f(double x){double y [[fisics::dim(length)]]=3;double t [[fisics::dim(time)]]='+op+'(x'+(',y' if op!='fabs' else '')+');}\n')
        cmd=[str(compiler),'--overlay=physics-units','-c',str(src),'-o',str(obj)]
        p=subprocess.run(cmd,cwd=ROOT,env=env,text=True,capture_output=True,timeout=60)
        (out/(name+'.log')).write_text(p.stdout+p.stderr)
        assert p.returncode==0 and obj.exists(),(name,p)
        obj.unlink();rows.append(dict(id=name,command=cmd,exit=0,classification='unchecked_control'))
    (out/'results.json').write_text(json.dumps(dict(status='PASS',cases=len(rows),results=rows),indent=2)+'\n')
    print(f"Math-helper contracts: {len(rows)} PASS (including 3 unchecked controls)")
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',required=True,type=Path);p.add_argument('--out',type=Path);a=p.parse_args()
    if a.out:
        a.out.mkdir(parents=True,exist_ok=False);check(a.compiler.resolve(),a.out.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='fisics-math-') as tmp:check(a.compiler.resolve(),Path(tmp))
if __name__=='__main__':main()
