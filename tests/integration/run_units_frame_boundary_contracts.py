#!/usr/bin/env python3
"""Typed frame releases/transforms and explicitly trusted dense exchange."""
import argparse
from fractions import Fraction as F
import hashlib
import json
import math
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
from run_units_frame_local_contracts import beam_matrix, sample_vector, close_number

ROOT=Path(__file__).resolve().parents[2]
KERNEL=ROOT/'tests/fixtures/solver_physics/frame_boundary.h'
RUNTIME=ROOT/'tests/final/cases/15__units_frame_boundary.c'
SUMMARY='Frame boundary: 60 cases, 2160 coefficient positions, 660 applications; PASS\n'
INPUTS=[[F(x) for x in row] for row in [
    ['12000000','.02','.0004','2'],['200000000000','.02','.00006666666666666667','2'],
    ['70000000000','.003','.0000012','.75']]]
BASES=[(F(1),F(0)),(F(0),F(1)),(F(-1),F(0)),(F(3,5),F(4,5)),(F(3,5),F(-4,5))]
MUTATIONS=[['pivot_class', 'original.rr[r][r];', 'original.tt[r][r];', 4103], ['condense_tt', 'original.tr[i][r]*(original.rt[r][j]/pivot);', 'original.rr[i%2][r]*(original.rt[r][j]/pivot);', 4102], ['condense_tr', 'original.tr[i][r]*(original.rr[r][j]/pivot);', 'original.tr[i][r]*(original.rr[r][j]/original.tt[0][0]);', 4102], ['condense_rt', 'original.rr[i][r]*(original.rt[r][j]/pivot);', 'original.tr[i][r]*(original.rt[r][j]/pivot);', 4102], ['condense_rr', 'original.rr[i][r]*(original.rr[r][j]/pivot);', 'original.tr[i][r]*(original.rr[r][j]/pivot);', 4102], ['transform_tt', 'R[a][i]*original.tt[a][b]*R[b][j];', 'R[a][i]*original.rr[a%2][b%2]*R[b][j];', 4101], ['transform_tr', 'R[a][i]*original.tr[a][j];', 'R[a][i]*original.tt[a][j];', 4101], ['transform_rt', 'original.rt[i][a]*R[a][j];', 'original.rr[i][a%2]*R[a][j];', 4101], ['transform_rr', 'out->rr[i][j]=original.rr[i][j];', 'out->rr[i][j]=original.tt[i][j];', 4103], ['coordinate_class', 'out->translation[a]=basis->cosine*original.translation[a]+basis->sine*original.translation[a+1];', 'out->translation[a]=basis->cosine*original.rotation[end];', 4103], ['force_class', 'out->force[a]=basis->cosine*original.force[a]-basis->sine*original.force[a+1];', 'out->force[a]=basis->cosine*original.moment[end];', 4103], ['moment_class', 'out->moment[end]=original.moment[end];', 'out->moment[end]=original.force[end];', 4103]]


def condensed(k,mask):
    """Exact simultaneous Schur complement, including the full 2x2 release block."""
    release=[i for i,flag in [(2,1),(5,2)] if mask&flag]
    if not release:return k
    if len(release)==1:
        inverse=[[1/k[release[0]][release[0]]]]
    else:
        a,b=release;det=k[a][a]*k[b][b]-k[a][b]*k[b][a]
        inverse=[[k[b][b]/det,-k[a][b]/det],[-k[b][a]/det,k[a][a]/det]]
    out=[[F(0) for _ in range(6)] for _ in range(6)]
    for i in range(6):
        for j in range(6):
            if i not in release and j not in release:
                out[i][j]=k[i][j]-sum(k[i][a]*inverse[x][y]*k[b][j]
                    for x,a in enumerate(release) for y,b in enumerate(release))
    return out


def global_matrix(section,mask,basis):
    k=condensed(beam_matrix(*section),mask);c,s=basis
    t=[[F(0) for _ in range(6)] for _ in range(6)]
    for base in [0,3]:
        t[base][base]=c;t[base][base+1]=s;t[base+1][base]=-s;t[base+1][base+1]=c;t[base+2][base+2]=1
    return [[sum(t[a][i]*k[a][b]*t[b][j] for a in range(6) for b in range(6))
        for j in range(6)] for i in range(6)]


def validate_values(output):
    cases,coeff,qvalues,loads,tags={},{},{},{},{}
    summaries=0
    for line in output.splitlines():
        f=line.split()
        if f[0]=='M':
            key=tuple(map(int,f[1:4]));assert key not in tags
            tags[key]=float(f[4])
        elif f[0]=='C':
            key=int(f[1]);assert key not in cases
            cases[key]=tuple(map(int,f[2:]));assert len(cases[key])==3
        elif f[0]=='K':
            key=tuple(map(int,f[1:5]));assert key not in coeff
            coeff[key]=float(f[5])
        elif f[0]=='Q':
            key=tuple(map(int,f[1:3]));assert key not in qvalues
            qvalues[key]=list(map(float,f[3:]))
        elif f[0]=='P':
            key=tuple(map(int,f[1:5]));assert key not in loads
            loads[key]=float(f[5])
        else:assert line+'\n'==SUMMARY;summaries+=1
    assert summaries==1 and len(cases)==60 and len(coeff)==6480 and len(qvalues)==660 and len(loads)==15840
    assert len(tags)==72
    for path in range(2):
        for i in range(6):
            for j in range(6):assert tags[path,i,j]==1000*path+6*i+j+1
    for si in range(3):
        for mask in range(4):
            for bi,basis in enumerate(BASES):
                case=(si*4+mask)*5+bi;assert cases[case]==(si,mask,bi)
                k=global_matrix(INPUTS[si],mask,basis)
                for path in range(3):
                    for i in range(6):
                        for j in range(6):close_number(coeff[case,path,i,j],k[i][j],abs(k[i][j]))
                for n in range(11):
                    q=sample_vector(n,INPUTS[si][3])
                    if n==9:q[3]=-basis[1]*INPUTS[si][3]*F('.001');q[4]=basis[0]*INPUTS[si][3]*F('.001')
                    assert len(qvalues[case,n])==6
                    for actual,expected in zip(qvalues[case,n],q):close_number(actual,expected,abs(expected))
                    for i in range(6):
                        terms=[k[i][j]*q[j] for j in range(6)]
                        for path in range(4):close_number(loads[case,n,path,i],sum(terms),sum(abs(t) for t in terms))
    return dict(dense_permutation_comparisons=72,cases=60,coefficient_positions=2160,coefficient_paths=3,coefficient_comparisons=6480,
        applications=660,result_components_per_path=3960,application_paths=4,result_comparisons=15840,
        oracle='rational Hermite integration, simultaneous Schur complement, and basis congruence',
        absolute_tolerance=1e-9,relative_tolerance=1e-10)


def check(compiler,out):
    rows=[];env={**os.environ,'FISICS_MAX_PROCS':'0'};clang=shutil.which('clang');assert clang
    include=['-I',ROOT/'include','-I',KERNEL.parent]
    def command(name,cmd,expected=0):
        p=subprocess.run(list(map(str,cmd)),cwd=ROOT,env=env,text=True,capture_output=True,timeout=120)
        (out/(name+'.log')).write_text(p.stdout+p.stderr)
        assert p.returncode==expected,(name,p.returncode,p.stdout,p.stderr)
        rows.append(dict(id=name,command=list(map(str,cmd)),exit=p.returncode));return p.stdout
    binaries=[]
    for label,tool,args in [('fisics',compiler,['--overlay=physics-units']),
        ('clang',clang,['-std=c99','-Wall','-Wextra','-Werror','-ffp-contract=off'])]:
        binary=out/label;binaries.append(binary)
        command(label+'-build',[tool,*args,*include,RUNTIME,'-lm','-o',binary])
        assert command(label+'-runtime',[binary])==SUMMARY
        data=command(label+'-values',[binary,'--values']);detail=validate_values(data)
        (out/(label+'-parity.json')).write_text(json.dumps(detail,indent=2)+'\n')
    data=(out/'fisics-values.log').read_text()
    for kind in ['coefficient','load','nonfinite']:
        lines=data.splitlines();prefix='P ' if kind=='load' else 'K '
        i=next(i for i,line in enumerate(lines) if line.startswith(prefix));fields=lines[i].split()
        fields[-1]='nan' if kind=='nonfinite' else str(float(fields[-1])+1)
        lines[i]=' '.join(fields)
        try:validate_values('\n'.join(lines)+'\n')
        except (AssertionError,ValueError):pass
        else:raise AssertionError('oracle accepted '+kind)
    base=KERNEL.read_text()
    for name,old,new,code in MUTATIONS:
        case=out/name;case.mkdir();assert base.count(old)==1,(name,old)
        (case/'frame_boundary.h').write_text(base.replace(old,new))
        src=case/'caller.c';src.write_text('#include "frame_boundary.h"\nint main(void){return 0;}\n')
        diag,obj=case/'diag.json',case/'caller.o'
        command(name,[compiler,'--overlay=physics-units',*include,'--emit-diags-json',diag,'-c',src,'-o',obj],1)
        assert not obj.exists()
        errors=[d for d in json.loads(diag.read_text())['diagnostics'] if d['severity_name']=='error']
        assert len(errors)==1 and errors[0]['code_id']==code,(name,errors)
        d=errors[0];assert d['category_name']=='extension' and d['line']>0 and d['column']>0
        assert len(d['details']['lhs_dim'])==8 and len(d['details']['rhs_dim'])==8,d
        assert d['details']['context']=={4101:'addition',4102:'subtraction',4103:'assignment'}[code],d
        control=case/'clang.o';command(name+'-clang',[clang,'-std=c99',*include,'-c',src,'-o',control]);control.unlink()
    codes=[4103,4103,4112,4113,4103,4103,4107,4103,4102]
    for variant,code in enumerate(codes,1):
        name=f'boundary_failure_{variant}';diag,obj=out/(name+'.json'),out/(name+'.o')
        command(name,[compiler,'--overlay=physics-units',*include,f'-DFAIL_KIND={variant}',
            '--emit-diags-json',diag,'-c',ROOT/'tests/final/cases/15__units_frame_boundary_failures.c','-o',obj],1)
        assert not obj.exists();errors=[d for d in json.loads(diag.read_text())['diagnostics'] if d['severity_name']=='error']
        assert len(errors)==1 and errors[0]['code_id']==code,(name,errors)
        d=errors[0];assert d['line']>0 and d['column']>0 and d['category_name']=='extension'
    numeric=[
        ('pack_transpose','dense->value[t[i]][t[j]]=k->tt[i][j];','dense->value[t[j]][t[i]]=k->tt[i][j];'),
        ('restore_transpose','k->tt[i][j]=dense->value[t[i]][t[j]];','k->tt[i][j]=dense->value[t[j]][t[i]];'),
        ('pack_mapping','dense->value[t[i]][t[j]]=k->tt[i][j];','dense->value[t[i]][t[j]]=k->rr[i%2][j%2];'),
        ('restore_mapping','k->tt[i][j]=dense->value[t[i]][t[j]];','k->tt[i][j]=dense->value[r[i%2]][r[j%2]];'),
        ('load_sign','basis->cosine*original.force[a]-basis->sine*original.force[a+1];','basis->cosine*original.force[a]+basis->sine*original.force[a+1];'),
        ('release_end','unsigned r=release_a?0:1;','unsigned r=release_a?1:0;'),
    ]
    runtime=RUNTIME.read_text();inc='#include "../../fixtures/solver_physics/frame_boundary.h"';assert runtime.count(inc)==1
    for name,old,new in numeric+[('raw_corruption','','')]:
        case=out/name;case.mkdir()
        if old:assert base.count(old)==1
        (case/'frame_boundary.h').write_text(base.replace(old,new) if old else base)
        text=runtime.replace(inc,'#include "frame_boundary.h"')
        if name=='raw_corruption':
            needle='frame_dense_pack(&global,&packed);';assert text.count(needle)==1
            text=text.replace(needle,needle+'\n        packed.value[0][0]=packed.value[2][2];')
        src=case/'runtime.c';src.write_text(text);binary=case/'bad'
        command(name+'-build',[compiler,'--overlay=physics-units',*include,src,'-lm','-o',binary])
        command(name+'-runtime',[binary],1);binary.unlink()
        rows[-2]['classification']='dimensionally_accepted_numerical_mutation'
    raw=out/'raw'
    command('original_raw_gap',[compiler,'--overlay=physics-units',*include,'-DBAD_MATRIX_SLOT',
        ROOT/'tests/fixtures/solver_physics/structure.c','-o',raw]);assert raw.exists();raw.unlink()
    rows[-1]['classification']='unchecked_control'
    for binary in binaries:binary.unlink()
    result=dict(status='PASS',commands=len(rows),required_physical_rejections=21,ordinary_c_mutation_controls=12,
        numerical_mutation_controls=7,oracle_failure_controls=3,unchecked_controls=1,parity=detail,
        compiler_sha256=hashlib.sha256(compiler.read_bytes()).hexdigest(),kernel_sha256=hashlib.sha256(KERNEL.read_bytes()).hexdigest(),results=rows)
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(f'Frame boundary: 60 cases, 6480 coefficient and 15840 load comparisons per compiler; '
        f'21 physical rejections / 7 numerical mutations / original raw gap PASS ({len(rows)} commands)')


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',required=True,type=Path);p.add_argument('--out',type=Path);a=p.parse_args()
    if a.out:a.out.mkdir(parents=True,exist_ok=False);check(a.compiler.resolve(),a.out.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='fisics-frame-boundary-') as tmp:check(a.compiler.resolve(),Path(tmp))
if __name__=='__main__':main()
