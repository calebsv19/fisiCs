#!/usr/bin/env python3
"""Qualify a hash-bound patch on actual external structural sources, or an already maintained owner against a frozen unannotated baseline."""
import argparse
import concurrent.futures
import hashlib
import json
import math
import os
from pathlib import Path
import shutil
import subprocess

ROOT=Path(__file__).resolve().parents[2]
FIX=ROOT/'tests/fixtures/solver_physics/structural_source'
KERNELS=[('cs_physical_build','element',(54,2,15)),('cs_physical_apply','recovery',(26,2,6)),
         ('cs_physical_equivalent_load','loads',(14,0,0)),('cs_physical_recover_end','recovery',(28,0,6))]
TESTS=['core_structural_frame_test','core_structural_test','core_structural_dynamic_test',
       'core_structural_dynamic_system_test','core_structural_transient_test','core_structural_support_test',
       'core_structural_membrane_test','core_structural_plate_test']

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def snapshot(root):
    paths=[p for part in ['src','include','tests'] for p in (root/part).rglob('*') if p.is_file() and p.suffix in ['.c','.h','.py']]
    return {str(p.relative_to(root)):sha(p) for p in sorted(paths)+[root/n for n in ['Makefile','VERSION','README.md']]}

def validate(output):
    """Independent closed-form cantilever oracle, plus full local 6x6 matrix."""
    observed={};abi=None
    for row in output.splitlines():
        f=row.split()
        if f[0]=='ABI':assert abi is None;abi=row
        elif f[0]=='C':
            assert len(f)==18
            for j,v in enumerate(f[2:]):observed['C',int(f[1]),j]=float(v)
        elif f[0]=='K':observed['K',int(f[1]),int(f[2])]=float(f[3])
        else:raise AssertionError(row)
    expected={};E=200e9;A=.1*.2;I=.1*.2**3/12;W=A*.2/6
    for n in range(9):
        L=1+(n%3)*1.5;P=1000 if n<3 or n>=6 else 0;q=0 if n<3 else 250
        moment=P*L+q*L*L/2;shear=P+q*L
        energy=P*P*L**3/(6*E*I)+P*q*L**4/(8*E*I)+q*q*L**5/(40*E*I)
        # For these monotone cantilevers the chord-relative maximum is checked
        # by the existing library test; avoid duplicating its root solver here.
        values=[-(P*L**3/(3*E*I)+q*L**4/(8*E*I)),-(P*L*L/(2*E*I)+q*L**3/(6*E*I)),
                shear,moment,moment/W,moment/W,1.5*shear/A,energy,2*energy]
        values += [None,0,shear,moment,0,-P,0]
        for j,v in enumerate(values):
            if v is not None:expected['C',n,j]=v
        a=E*A/L;b=E*I/L**3
        K=[[a,0,0,-a,0,0],[0,12*b,6*b*L,0,-12*b,6*b*L],
           [0,6*b*L,4*b*L*L,0,-6*b*L,2*b*L*L],[-a,0,0,a,0,0],
           [0,-12*b,-6*b*L,0,12*b,-6*b*L],[0,6*b*L,2*b*L*L,0,-6*b*L,4*b*L*L]]
        for i in range(36):expected['K',n,i]=K[i//6][i%6]
    assert abi and len(observed)==9*(16+36),len(observed)
    for key,v in expected.items():
        actual=observed[key];assert math.isfinite(actual) and abs(actual-v)<=1e-8+1e-8*abs(v),(key,actual,v)
    assert all(math.isfinite(v) for v in observed.values())
    return dict(abi=abi,values=observed,analytic_values=len(expected))

def check(compiler,source,base,out,physical=False,mechanical=False,maintained_baseline=None,structural_policy=None,boundary_adoption=False,energy_adoption=False):
    physical=physical or mechanical
    assert not energy_adoption or boundary_adoption, 'energy adoption requires maintained P7.4 boundary baseline'
    assert not boundary_adoption or (maintained_baseline and structural_policy), 'boundary adoption requires frozen baseline and v7 policy'
    assert not structural_policy or (maintained_baseline and not physical), 'structural policy requires maintained mode'
    structural_model={str(p):sha(p) for p in [structural_policy,structural_policy.parent/'frame_catalog.json']} if structural_policy else {}
    semantic_args=['--overlay=physics-semantics','--physical-semantics-policy',str(structural_policy),'-DCORE_STRUCTURAL_PHYSICAL_SEMANTICS=1'] if structural_policy else []
    env={**os.environ,'FISICS_MAX_PROCS':'0','PYTHONDONTWRITEBYTECODE':'1','TMPDIR':str(out/'tmp')}
    (out/'tmp').mkdir();commands=[];result=dict(schema=('fisics.structural_maintained_owner.v1' if maintained_baseline else 'fisics.structural_source_candidate.v1'),passed=False,
        structural_model_sha256=structural_model,adopted=maintained_baseline is not None,compiler_sha256=sha(compiler),runner_sha256=sha(Path(__file__)),commands=commands,coverage=[])
    def run(label,cmd,cwd=out,code=0):
        p=subprocess.run(list(map(str,cmd)),cwd=cwd,env=env,capture_output=True,text=True,timeout=180)
        (out/(label+'.log')).write_text(p.stdout+p.stderr)
        commands.append(dict(label=label,command=list(map(str,cmd)),cwd=str(cwd),exit=p.returncode,expected=code))
        assert p.returncode==code,(label,p.returncode,(p.stdout+p.stderr)[-2000:])
        # Some unsupported backend paths print errors without a failing exit.
        assert 'Error:' not in p.stdout+p.stderr or code!=0,(label,'backend error despite exit zero')
        return p.stdout
    def report(root,label,fn,part,code=0,declared=False):
        path=out/(label+'.json')
        run(label,[compiler,'--overlay=physics-units',*includes(root),'--units-coverage-strict',
            '--units-coverage-allow-literal-assumptions',*(['--units-coverage-allow-declared-boundaries'] if declared else []),'--units-coverage-function',fn,
            '--emit-units-coverage-json',path,root/f'src/core_structural_frame_{part}.c'],code=code)
        data=json.loads(path.read_text());policy=data['strict_policy']
        assert data['scope']['name']==fn
        if code==0:
            assert data['scope']['enumeration_complete'] and data['scope']['unsupported_nodes']==0
        assert data['denominator']['total']==len(data['obligations'])
        assert policy['accepted'] is (code==0)
        assert policy['required']==policy['checked']+policy['permitted_trust']+policy['blocked']
        result['coverage'].append(dict(label=label,function=fn,policy=policy))
        return policy
    def includes(root):return ['-I',ROOT/'include','-I',root/'include','-I',root/'src','-I',base/'include']
    def build(root,tool,label):
        dest=out/label;dest.mkdir();s=(root/'Makefile').read_text()
        s=s[s.index('SOURCES :=')+len('SOURCES :='):s.index('\nOBJECTS :=')].replace('\\\n',' ')
        sources=[Path(x.replace('$(BASE_ROOT)',str(base))) if x.startswith('$(BASE_ROOT)') else root/x for x in s.split()]
        assert len({p.name for p in sources})==len(sources),'object name collision'
        args=[tool,*((semantic_args if structural_policy else (['--overlay=physics-semantics','--physical-semantics-policy',str(out/'policy.json')] if physical else ['--overlay=physics-units'])) if tool==compiler else ['-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O0','-ffp-contract=off']),
              *includes(root),'-DCORE_STRUCTURAL_VERSION="'+(root/'VERSION').read_text().strip()+'"']
        def obj(p):
            target=dest/(p.stem+'.o');run(label+'-'+p.stem,[*args,'-c',p,'-o',target]);return target
        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:objects=list(pool.map(obj,sources))
        archive=dest/'structural.a';run(label+'-archive',['ar','rcs',archive,*objects]);outputs={}
        for name in TESTS+['structural_source_probe']:
            p=FIX/'probe.c' if name=='structural_source_probe' else root/'tests'/(name+'.c')
            target=obj(p);binary=dest/name
            run(label+'-'+name+'-link',['clang',target,archive,'-lm','-o',binary])
            outputs[name]=run(label+'-'+name+'-run',[binary])
        return outputs
    try:
        entry=snapshot(source);base_hash={str(p.relative_to(base)):sha(p) for part in ['src','include'] for p in (base/part).rglob('*') if p.is_file()}
        patch_fixture=ROOT/'tests/fixtures/solver_physics/structural_physical' if physical else FIX
        baseline=maintained_baseline or source
        baseline_entry=snapshot(baseline)
        if maintained_baseline is None:
            pins=json.loads((patch_fixture/'patch_inputs.json').read_text())
            for name,digest in pins.items():assert (sha(source/name) if (source/name).exists() else None)==digest,('patch source drift',name)
        else:
            assert not physical, 'maintained dimensional gate does not generate a semantic profile'
            assert (baseline/'src/core_structural_frame_physical.h').exists() is bool(boundary_adoption), 'baseline must match the selected adoption checkpoint'
            allowed={'src/core_structural_frame_physical.h',*[f'src/core_structural_frame_{p}.c' for p in ['element','loads','recovery']]}
            if boundary_adoption:
                allowed={'src/core_structural_frame_boundaries.h',*[f'src/core_structural_frame_{p}.c' for p in ['element','loads','recovery','assemble','linear','result']]}
            if energy_adoption:
                allowed={'src/core_structural_frame_energy.h','src/core_structural_frame_result.c','src/core_structural_frame_recovery.c'}
            changed={n for n in set(entry)|set(baseline_entry) if n!='README.md' and entry.get(n)!=baseline_entry.get(n)}
            assert changed==allowed, ('unexpected adoption delta', changed)
        original=out/'original';original.mkdir()
        for part in ['src','include','tests']:shutil.copytree(baseline/part,original/part,ignore=shutil.ignore_patterns('__pycache__','*.pyc'))
        for name in ['Makefile','VERSION','README.md']:shutil.copy2(baseline/name,original/name)
        assert snapshot(original)==baseline_entry,'baseline changed while freezing'
        candidate=out/'candidate'
        if maintained_baseline is None:
            shutil.copytree(original,candidate)
            run('patch-check',['git','apply','--check',patch_fixture/'candidate.patch'],candidate)
            run('patch-apply',['git','apply',patch_fixture/'candidate.patch'],candidate)
        else:
            candidate.mkdir()
            for part in ['src','include','tests']:shutil.copytree(source/part,candidate/part,ignore=shutil.ignore_patterns('__pycache__','*.pyc'))
            for name in ['Makefile','VERSION','README.md']:shutil.copy2(source/name,candidate/name)
            assert snapshot(candidate)==entry,'maintained source changed while freezing'
            result['baseline_sha256']=baseline_entry
            mapping={fn:f'src/core_structural_frame_{part}.c' for fn,part,_ in KERNELS}
            for fn,name in mapping.items():
                assert (candidate/name).read_text().count(fn+'(')==1, ('missing maintained call',fn)
            result['maintained_call_sites']=mapping
        result.update(source_root=str(source),source_sha256=entry,base_sha256=base_hash,candidate_sha256=snapshot(candidate),patch_sha256=None if maintained_baseline else sha(patch_fixture/'candidate.patch'))
        for fn,part,expected in KERNELS:
            p=report(candidate,fn,fn,part,declared=bool(structural_policy and fn in ["cs_physical_build","cs_physical_equivalent_load"]))
            if maintained_baseline is None:
                assert (p['checked'],p['permitted_trust'],p['ordinary_controls'])==expected
            else:
                reviewed=({'cs_physical_build':(57,11,15),'cs_physical_equivalent_load':(13,5,0)}.get(fn,expected) if structural_policy else ((57,3,15) if fn=='cs_physical_build' else expected))
                assert (p['checked'],p['permitted_trust'],p['ordinary_controls'])==reviewed
            assert p['blocked']==0
        p=report(candidate,'raw-restoration-refused','cs_physical_dense_restore_assumed_si','recovery',1,declared=True)
        assert p['blocked']>0 and p['checked']==0
        # True type mistakes must remain legal ordinary C and directly reject.
        h=candidate/'src/core_structural_frame_physical.h';text=h.read_text()
        mutations=[('construction','6 * bending * section->length;','6 * bending;','cs_physical_build','element'),
                   ('application','k->tr[i][0] * q->rotation[0];','k->tr[i][0] * q->translation[0];','cs_physical_apply','recovery'),
                   ('equivalent','q->y*q->length*q->length/12;','q->y*q->length/12;','cs_physical_equivalent_load','loads'),
                   ('recovery','-equivalent->force[i];','-equivalent->moment[i%2];','cs_physical_recover_end','recovery')]
        if structural_policy:
            mutations[0]=('construction','six * bending * section->length','six * bending','cs_physical_build','element')
            mutations[2]=('equivalent','cs_physical_line_moment(q->y*q->length,q->length)/twelve','q->y*q->length/twelve','cs_physical_equivalent_load','loads')
        for label,old,new,fn,part in mutations:
            assert text.count(old)==(2 if maintained_baseline and label=='construction' else 1),label
            h.write_text(text.replace(old,new,1))
            src=candidate/f'src/core_structural_frame_{part}.c'
            run(label+'-clang',['clang','-std=c11','-pedantic-errors',*includes(candidate),'-fsyntax-only',src])
            run(label+'-overlay',[compiler,'--overlay=physics-units',*includes(candidate),'-c',src,'-o',out/(label+'.o')],code=1)
            report(candidate,label+'-strict',fn,part,1)
        import re
        stripped,n=re.subn(r'double translation\[4\][^;]+;', 'double translation[4];', text)
        assert n==1
        h.write_text(stripped)
        p=report(candidate,'missing-coordinate-metadata','cs_physical_apply','recovery',1,declared=True);assert p['blocked']>0
        h.write_text(text)
        if physical:
            if mechanical:
                from solver_mechanical_profile import apply_mechanical_profile as apply_profile
            else:
                from solver_physical_profile import apply_profile
            result['physical_profile']=apply_profile(candidate,out)
            result['physical_profile_mode']='mechanical-v4' if mechanical else 'legacy-v1'
            result['physical_model_sha256']={n:sha(out/n) for n in ['policy.json','catalog.json']}
            result['candidate_sha256']=snapshot(candidate)
            text=h.read_text()
        outputs={}
        for label,root,tool in [('baseline-clang',original,Path(shutil.which('clang'))),('candidate-clang',candidate,Path(shutil.which('clang'))),('candidate-fisics',candidate,compiler)]:
            outputs[label]=build(root,tool,label)
        parsed={k:validate(v['structural_source_probe']) for k,v in outputs.items()}
        reference=parsed['baseline-clang'];assert all(d['abi']==reference['abi'] for d in parsed.values())
        for d in parsed.values():
            assert d['values'].keys()==reference['values'].keys()
            for key,v in d['values'].items():assert abs(v-reference['values'][key])<=1e-8+1e-8*abs(reference['values'][key]),(key,v,reference['values'][key])
        result.update(outputs=outputs,abi=reference['abi'],analytic_values_per_variant=reference['analytic_values'],numerical_values_per_variant=len(reference['values']))
        # A legal dimensioned coefficient mutation must fail a real solver test.
        h.write_text(text.replace('= four * bending * section->length' if structural_policy else '= 4 * bending * section->length', '= (four + (two/two)) * bending * section->length' if structural_policy else '= 5 * bending * section->length'))
        assert h.read_text()!=text
        mutated=out/'mutated-element.o'
        run('numeric-mutant-compile',[compiler,*(semantic_args if structural_policy else (['--overlay=physics-semantics','--physical-semantics-policy',str(out/'policy.json')] if physical else ['--overlay=physics-units'])),*includes(candidate),'-c',candidate/'src/core_structural_frame_element.c','-o',mutated])
        # ar replaces by object basename, retaining all other candidate objects.
        replacement=out/'numeric-mutant';replacement.mkdir()
        shutil.copy2(mutated,replacement/'core_structural_frame_element.o')
        archive=replacement/'structural.a';shutil.copy2(out/'candidate-fisics/structural.a',archive)
        run('numeric-mutant-archive',['ar','r',archive,replacement/'core_structural_frame_element.o'])
        binary=replacement/'frame-test'
        run('numeric-mutant-link',['clang',out/'candidate-fisics/core_structural_frame_test.o',archive,'-lm','-o',binary])
        failure=run('numeric-mutant-rejected',[binary],code=1)
        h.write_text(text)
        result['dimensionally_legal_numeric_mutant_rejected']=True
        assert snapshot(candidate)==result['candidate_sha256'],'candidate drift'
        assert snapshot(source)==entry,'external owner source drift'
        assert snapshot(baseline)==baseline_entry,'baseline drift'
        assert {str(p.relative_to(base)):sha(p) for part in ['src','include'] for p in (base/part).rglob('*') if p.is_file()}==base_hash,'core_base drift'
        assert all(sha(Path(p))==h for p,h in structural_model.items()),'structural model drift'
        result['passed']=True
        print(('Maintained structural owner' if maintained_baseline else 'Actual structural source candidate')+': PASS; 4 strict kernels, 4 type mutations, metadata/raw refusals, 3 numerical variants, 8 existing tests/variant')
    finally:(out/'results.json').write_text(json.dumps(result,indent=2)+'\n')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for n in ['compiler','source-root','base-root','out']:p.add_argument('--'+n,type=Path,required=True)
    p.add_argument('--energy-adoption',action='store_true',help='P7.5: P7.4 baseline; only recovery/publication and private energy helper change (also requires --boundary-adoption)')
    p.add_argument('--boundary-adoption',action='store_true',help='P7.4: baseline already has P7.3 carriers; only six static call sites and the private boundary header may change')
    p.add_argument('--structural-policy',type=Path,help='P7.3 maintained v7 model; enables source semantic opt-in for numerical builds')
    p.add_argument('--maintained-baseline',type=Path,help='Frozen unannotated owner; qualify already adopted source without applying a patch')
    p.add_argument('--physical',action='store_true',help='Add the explicit P5.6 semantic candidate profile after units qualification')
    p.add_argument('--mechanical',action='store_true',help='Use the refined v4 mechanical profile (implies --physical); never adopts external sources')
    a=p.parse_args();a.out.mkdir(parents=True,exist_ok=False)
    check(a.compiler.resolve(),a.source_root.resolve(),a.base_root.resolve(),a.out.resolve(),a.physical,a.mechanical,a.maintained_baseline.resolve() if a.maintained_baseline else None,a.structural_policy.resolve() if a.structural_policy else None,a.boundary_adoption,a.energy_adoption)
if __name__=='__main__':main()
