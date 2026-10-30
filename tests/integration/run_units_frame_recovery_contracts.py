#!/usr/bin/env python3
"""Qualify typed local loads, joints, force/stress recovery and work/energy."""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
from solver_frame_recovery_oracle import expected_values

ROOT=Path(__file__).resolve().parents[2]
FIXTURES=ROOT/'tests/fixtures/solver_physics'
RUNTIME=ROOT/'tests/final/cases/15__units_frame_recovery.c'
HEADERS=['frame_loading.h','frame_joint.h','frame_recovery.h']
SUMMARY='Frame recovery: 36 load cases, 72 members, 16 joints; PASS\n'
MUTATIONS=[['density_area', 'input->density*s->area;', 'input->density;', 4103], ['load_moment_length', 'q->component[1]*s->length*s->length/12;', 'q->component[1]*s->length/12;', 4103], ['release_rotation_class', 'rhs->moment[0]/k->rr[0][0];', 'rhs->moment[0]/k->tt[0][0];', 4103], ['condensed_load_class', 'original.force[i]-k->tr[i][0]*rotation.value[0]', 'original.force[i]-k->rr[i%2][0]*rotation.value[0]', 4102], ['nodal_work_class', 'q->rotation[i]*p->moment[i];', 'q->rotation[i]*p->force[i];', 4101], ['joint_tt_class', 'k->tt[a][a]=s->translation[a];', 'k->tt[a][a]=s->rotation;', 4103], ['joint_rr_class', 'k->rr[0][0]=s->rotation;', 'k->rr[0][0]=s->translation[0];', 4103], ['joint_force_class', 's->translation[a]*out->relative_translation[a];', 's->rotation*out->relative_translation[a];', 4103], ['joint_energy_length', '.5*out->force_on_a[a]*out->relative_translation[a];', '.5*out->force_on_a[a];', 4101], ['joint_moment_class', 's->rotation*out->relative_rotation;', 's->translation[0]*out->relative_rotation;', 4103], ['stress_modulus', 'sign*f->moment[0]/modulus;', 'sign*f->moment[0]/area;', 4101], ['shear_length', 'ei/s->length/s->length/s->length*6*f->transverse_displacement[3];', 'ei/s->length/s->length*6*f->transverse_displacement[3];', 4103], ['moment_length', 'ei/s->length/s->length*2*f->transverse_displacement[2];', 'ei/s->length/s->length/s->length*2*f->transverse_displacement[2];', 4103], ['bubble_length', 'line->component[1]*s->length*s->length*s->length*s->length/(24*ei);', 'line->component[1]*s->length*s->length*s->length/(24*ei);', 4103], ['axial_bubble_class', 'line->component[0]*s->length*s->length/(2*ea);', 'line->component[0]*s->length*s->length/(2*ei);', 4103], ['member_energy_length', 'f->energy=s->length*(axial_integral+moment_integral)/2;', 'f->energy=(axial_integral+moment_integral)/2;', 4103], ['distributed_work_length', 'f->distributed_work=s->length*work_integral;', 'f->distributed_work=work_integral;', 4103], ['end_force_class', 'elastic.force[i]-load.force[i];', 'elastic.force[i]-load.moment[i%2];', 4102], ['work_return', 'return work;', 'return p->force[0];', 4113]]


def validate(output):
    observed={};summary=0
    for line in output.splitlines():
        f=line.split()
        if f[0] in ['B','R','J','A']:
            key=(f[0],int(f[1]),int(f[2]));value=float(f[3])
        elif f[0]=='K':key=('K',int(f[1]),int(f[2]),int(f[3]));value=float(f[4])
        else:assert line+'\n'==SUMMARY,line;summary+=1;continue
        assert key not in observed,key
        observed[key]=value
    expected=expected_values();assert summary==1 and observed.keys()==expected.keys(),(len(observed),len(expected))
    for key,v in expected.items():
        actual=observed[key];target=float(v)
        assert math.isfinite(actual) and abs(actual-target)<=1e-8+1e-9*abs(target),(key,actual,target)
    return dict(values=len(expected),load_cases=36,members=72,joints=16,spring_coefficients=576,
        member_values=2448,load_values=936,joint_values=224,analytical_values=8,
        absolute_tolerance=1e-8,relative_tolerance=1e-9,
        oracle='rational Hermite/particular-solution fields, exact polynomial integration and stationary stress extrema')


def check(compiler,out):
    env={**os.environ,'FISICS_MAX_PROCS':'0'};rows=[];clang=shutil.which('clang');assert clang
    includes=['-I',ROOT/'include','-I',FIXTURES]
    def command(name,cmd,expected=0):
        p=subprocess.run(list(map(str,cmd)),cwd=ROOT,env=env,text=True,capture_output=True,timeout=120)
        (out/(name+'.log')).write_text(p.stdout+p.stderr)
        if expected is not None:assert p.returncode==expected,(name,p.returncode,p.stdout,p.stderr)
        rows.append(dict(id=name,command=list(map(str,cmd)),exit=p.returncode));return p
    binaries=[]
    for label,tool,args in [('fisics',compiler,['--overlay=physics-units']),
        ('clang',clang,['-std=c99','-Wall','-Wextra','-Werror','-Wno-unused-function','-ffp-contract=off'])]:
        binary=out/label;binaries.append(binary)
        command(label+'-build',[tool,*args,*includes,RUNTIME,'-lm','-o',binary])
        assert command(label+'-runtime',[binary]).stdout==SUMMARY
        values=command(label+'-values',[binary,'--values']).stdout
        parity=validate(values);(out/(label+'-parity.json')).write_text(json.dumps(parity,indent=2)+'\n')
    original=(out/'fisics-values.log').read_text()
    for kind,prefix in [('wrong_load','B '),('wrong_stress','R 0 27 '),('nonfinite','J ')]:
        lines=original.splitlines();i=next(i for i,s in enumerate(lines) if s.startswith(prefix));fields=lines[i].split()
        fields[-1]='nan' if kind=='nonfinite' else str(float(fields[-1])+max(1,abs(float(fields[-1]))*.01))
        lines[i]=' '.join(fields)
        try:validate('\n'.join(lines)+'\n')
        except (AssertionError,ValueError):pass
        else:raise AssertionError('oracle accepted '+kind)
    base={name:(FIXTURES/name).read_text() for name in HEADERS}
    def stage(name,old,new,runtime=False):
        case=out/name;case.mkdir();assert sum(t.count(old) for t in base.values())==1,(name,old)
        for filename,text in base.items():(case/filename).write_text(text.replace(old,new))
        src=case/('runtime.c' if runtime else 'caller.c')
        if runtime:
            text=RUNTIME.read_text().replace('#include "../../fixtures/solver_physics/frame_recovery.h"','#include "frame_recovery.h"').replace('#include "../../fixtures/solver_physics/frame_joint.h"','#include "frame_joint.h"')
        else:text='#include "frame_joint.h"\n#include "frame_recovery.h"\nint main(void){return 0;}\n'
        src.write_text(text);return case,src
    for name,old,new,code in MUTATIONS:
        case,src=stage(name,old,new);diag,obj=case/'diag.json',case/'caller.o'
        command(name,[compiler,'--overlay=physics-units',*includes,'--emit-diags-json',diag,'-c',src,'-o',obj],1)
        assert not obj.exists()
        errors=[d for d in json.loads(diag.read_text())['diagnostics'] if d['severity_name']=='error']
        assert len(errors)==1 and errors[0]['code_id']==code,(name,errors)
        d=errors[0];assert d['category_name']=='extension' and d['line']>0 and d['column']>0
        assert len(d['details']['lhs_dim'])==len(d['details']['rhs_dim'])==8 and d['details']['lhs_dim']!=d['details']['rhs_dim'],d
        control=case/'clang.o';command(name+'-clang',[clang,'-std=c99',*includes,'-c',src,'-o',control]);control.unlink()
    codes=[4103,4103,4112,4112,4113,4107,4103,4103,4103]
    for variant,code in enumerate(codes,1):
        name=f'recovery_failure_{variant}';diag,obj=out/(name+'.json'),out/(name+'.o')
        command(name,[compiler,'--overlay=physics-units',*includes,f'-DFAIL_KIND={variant}',
            '--emit-diags-json',diag,'-c',ROOT/'tests/final/cases/15__units_frame_recovery_failures.c','-o',obj],1)
        assert not obj.exists();errors=[d for d in json.loads(diag.read_text())['diagnostics'] if d['severity_name']=='error']
        assert len(errors)==1 and errors[0]['code_id']==code,(name,errors)
        d=errors[0];assert d['line']>0 and d['column']>0 and d['category_name']=='extension'
    numeric=[
        ('line_sign','-basis->sine*gx+basis->cosine*gy+input->local.component[1];','basis->sine*gx+basis->cosine*gy+input->local.component[1];'),
        ('equivalent_factor','q->component[1]*s->length*s->length/12;','q->component[1]*s->length*s->length/6;'),
        ('joint_sign','s->translation[a]*out->relative_translation[a];','-s->translation[a]*out->relative_translation[a];'),
        ('joint_half','.5*out->moment_on_a*out->relative_rotation;','out->moment_on_a*out->relative_rotation;'),
        ('end_sign','elastic.force[i]-load.force[i];','elastic.force[i]+load.force[i];'),
        ('stress_interior','xi[count++]=interior;','xi[2]=interior;'),
        ('work_factor','f->distributed_work=s->length*work_integral;','f->distributed_work=2*s->length*work_integral;'),
        ('bubble_factor','/(24*ei);','/(12*ei);'),
    ]
    for name,old,new in numeric:
        case,src=stage(name,old,new,True);binary=case/'bad'
        command(name+'-build',[compiler,'--overlay=physics-units',*includes,src,'-lm','-o',binary])
        result=command(name+'-runtime',[binary,'--values'],None);assert result.returncode in [0,1]
        if result.returncode==0:
            try:validate(result.stdout)
            except (AssertionError,ValueError):pass
            else:raise AssertionError('numerical mutation escaped '+name)
        rows[-1]['classification']='numerically_rejected_after_physical_compile';binary.unlink()
    raw=out/'raw';command('raw_gap',[compiler,'--overlay=physics-units',*includes,'-DBAD_MATRIX_SLOT',FIXTURES/'structure.c','-o',raw]);assert raw.exists();raw.unlink();rows[-1]['classification']='unchecked_control'
    for binary in binaries:binary.unlink()
    result=dict(status='PASS',commands=len(rows),required_physical_rejections=28,ordinary_c_mutation_controls=19,numerical_mutation_controls=8,
        oracle_failure_controls=3,unchecked_controls=1,parity=parity,compiler_sha256=hashlib.sha256(compiler.read_bytes()).hexdigest(),
        inputs={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [RUNTIME,*[FIXTURES/n for n in HEADERS],Path(__file__).with_name('solver_frame_recovery_oracle.py')]},results=rows)
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(f'Frame recovery: 4192 exact-reference values per compiler; 28 physical rejections / 8 numerical mutations PASS ({len(rows)} commands)')


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',required=True,type=Path);p.add_argument('--out',type=Path);a=p.parse_args()
    if a.out:a.out.mkdir(parents=True,exist_ok=False);check(a.compiler.resolve(),a.out.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='fisics-frame-recovery-') as tmp:check(a.compiler.resolve(),Path(tmp))
if __name__=='__main__':main()
