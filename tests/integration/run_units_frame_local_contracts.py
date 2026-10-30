#!/usr/bin/env python3
"""Qualify the typed unreleased local frame boundary; raw matrices stay unchecked."""
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

ROOT = Path(__file__).resolve().parents[2]
KERNEL = ROOT / 'tests/fixtures/solver_physics/frame_local.h'
RUNTIME = ROOT / 'tests/final/cases/15__units_frame_local.c'
SUMMARY = 'Frame local: 3 sections, 108 coefficients, 33 applications, 198 result components; PASS\n'
TT, FORCE, MOMENT = [0,1,-2,0,0,0,0,0], [1,1,-2,0,0,0,0,0], [2,1,-2,0,0,0,0,0]
MUTATIONS = [
    ('build_tt_class','k->tt[0][0] = axial;','k->tt[0][0] = rotation_diagonal;',4103,TT,MOMENT),
    ('build_tr_class','k->tr[1][0] = coupling;','k->tr[1][0] = bending;',4103,FORCE,TT),
    ('build_rt_class','k->rt[0][1] = coupling;','k->rt[0][1] = rotation_diagonal;',4103,FORCE,MOMENT),
    ('build_rr_class','k->rr[0][0] = rotation_diagonal;','k->rr[0][0] = axial;',4103,MOMENT,TT),
    ('axial_missing_division','section->young * section->area / section->length;','section->young * section->area * section->length;',4103,TT,MOMENT),
    ('bending_missing_length','(section->length * section->length * section->length);','(section->length * section->length);',4103,TT,FORCE),
    ('apply_tt_rotation','k->tt[i][j] * q->translation[j];','k->tt[i][j] * q->rotation[j%2];',4101,FORCE,TT),
    ('apply_tr_translation','k->tr[i][j] * q->rotation[j];','k->tr[i][j] * q->translation[j];',4101,FORCE,MOMENT),
    ('apply_rt_rotation','k->rt[i][j] * q->translation[j];','k->rt[i][j] * q->rotation[j%2];',4101,MOMENT,FORCE),
    ('apply_rr_translation','k->rr[i][j] * q->rotation[j];','k->rr[i][j] * q->translation[j];',4101,MOMENT,[3,1,-2,0,0,0,0,0]),
]


def beam_matrix(young, area, inertia, length):
    """Exact integration of axial and cubic Hermite curvature operators.

    This does not copy the constructor's block assignments or its 6x6 table.
    Each curvature is a + b*xi. Integrate products over xi in [0,1].
    """
    k = [[F(0) for _ in range(6)] for _ in range(6)]
    axial = young*area/length
    for i, si in [(0, 1), (3, -1)]:
        for j, sj in [(0, 1), (3, -1)]:
            k[i][j] = axial*si*sj
    curvature = [(-6/length**2,12/length**2),(-4/length,6/length),
                 (6/length**2,-12/length**2),(-2/length,6/length)]
    for i, (a,b) in zip([1,2,4,5],curvature):
        for j, (c,d) in zip([1,2,4,5],curvature):
            k[i][j] = young*inertia*length*(a*c+(a*d+b*c)/2+b*d/3)
    return k


def sample_vector(n, length):
    q = [F(0) for _ in range(6)]
    if n < 6:
        q[n] = F(1)
    elif n == 6:
        q = [F('.001'),F('-.002'),F('.005'),F('.003'),F('.004'),F('-.006')]
    elif n == 7:
        q[0] = q[3] = F(1)
    elif n == 8:
        q[1] = q[4] = F(1)
    elif n == 9:
        q[2] = q[5] = F('.001');q[4] = length*F('.001')
    else:
        assert n == 10
        q[3] = F('.001')
    return q


def close_number(actual, expected, scale):
    assert math.isfinite(actual)
    assert abs(actual-float(expected)) <= 1e-9+1e-10*float(scale), (actual,float(expected),float(scale))


def validate_values(output):
    sections, coefficients, coordinates, loads = {}, {}, {}, {}
    for line in output.splitlines():
        fields = line.split()
        if fields[0] == 'S':
            key = int(fields[1]);assert key not in sections
            sections[key] = [F(x) for x in fields[2:]]
        elif fields[0] == 'K':
            key = tuple(map(int,fields[1:4]));assert key not in coefficients
            coefficients[key] = float(fields[4])
        elif fields[0] == 'Q':
            key = tuple(map(int,fields[1:3]));assert key not in coordinates
            coordinates[key] = list(map(float,fields[3:]))
        elif fields[0] == 'P':
            key = tuple(map(int,fields[1:4]));assert key not in loads
            loads[key] = float(fields[4])
        else:
            assert line+'\n' == SUMMARY, line
    inputs = [[F(x) for x in row] for row in [
        ['12000000','.02','.0004','2'],
        ['200000000000','.02','.00006666666666666667','2'],
        ['70000000000','.003','.0000012','.75']]]
    assert set(sections) == set(range(3)) and len(coefficients) == 108
    assert len(coordinates) == 33 and len(loads) == 198
    for c in range(3):
        assert len(sections[c]) == 4
        for observed, expected in zip(sections[c],inputs[c]):close_number(float(observed),expected,abs(expected))
        k = beam_matrix(*inputs[c]);length = inputs[c][3]
        for i in range(6):
            for j in range(6):close_number(coefficients[c,i,j],k[i][j],abs(k[i][j]))
        for n in range(11):
            q = sample_vector(n,length)
            assert len(coordinates[c,n]) == 6
            for actual, expected in zip(coordinates[c,n],q):close_number(actual,expected,abs(expected))
            for i in range(6):
                terms = [k[i][j]*q[j] for j in range(6)]
                close_number(loads[c,n,i],sum(terms),sum(abs(x) for x in terms))
    return dict(sections=3,coefficients=108,applications=33,result_components=198,
                oracle='exact rational axial/Hermite-curvature integration',absolute_tolerance=1e-9,relative_tolerance=1e-10)


def check(compiler, out):
    env = {**os.environ,'FISICS_MAX_PROCS':'0'}
    rows = []
    clang = shutil.which('clang');assert clang,'Clang control compiler is required'
    def command(name, cmd, expected=0):
        p = subprocess.run(list(map(str,cmd)),cwd=ROOT,env=env,text=True,capture_output=True,timeout=120)
        (out/(name+'.log')).write_text(p.stdout+p.stderr)
        assert p.returncode == expected,(name,p.returncode,p.stdout,p.stderr)
        rows.append(dict(id=name,command=list(map(str,cmd)),exit=p.returncode))
        return p.stdout
    binaries = []
    for label,tool,args in [('fisics',compiler,['--overlay=physics-units']),
                            ('clang',clang,['-std=c99','-Wall','-Wextra','-Werror','-ffp-contract=off'])]:
        binary = out/label
        command(label+'-build',[tool,*args,'-I',ROOT/'include',RUNTIME,'-lm','-o',binary])
        binaries.append(binary)
        assert command(label+'-runtime',[binary]) == SUMMARY
        values = command(label+'-values',[binary,'--values'])
        detail = validate_values(values)
        (out/(label+'-parity.json')).write_text(json.dumps(detail,indent=2)+'\n')
    # The oracle must refuse wrong coefficients, wrong results, and nonfinite data.
    original = (out/'fisics-values.log').read_text()
    for needle,replacement in [('K 0 0 0 120000','K 0 0 0 120001'),
                               ('P 0 10 3 120','P 0 10 3 121'),
                               ('K 0 0 0 120000','K 0 0 0 nan')]:
        assert needle in original
        try:validate_values(original.replace(needle,replacement,1))
        except (AssertionError,ValueError):pass
        else:raise AssertionError('oracle accepted deliberately invalid output')
    base = KERNEL.read_text()
    for name,old,new,code,lhs,rhs in MUTATIONS:
        case = out/name;case.mkdir()
        assert base.count(old) == 1,(name,old)
        (case/'frame_local.h').write_text(base.replace(old,new))
        source = case/'caller.c';source.write_text('#include "frame_local.h"\nint main(void){return 0;}\n')
        diag,obj = case/'diag.json',case/'caller.o'
        command(name,[compiler,'--overlay=physics-units','-I',ROOT/'include','--emit-diags-json',diag,'-c',source,'-o',obj],1)
        assert not obj.exists(),name
        errors = [d for d in json.loads(diag.read_text())['diagnostics'] if d['severity_name']=='error']
        assert len(errors)==1 and errors[0]['code_id']==code,(name,errors)
        error = errors[0];assert error['line']>0 and error['column']>0 and error['category_name']=='extension'
        assert error['details']['lhs_dim']==lhs and error['details']['rhs_dim']==rhs,(name,error)
        assert error['details']['context']==('assignment' if code==4103 else 'addition'),error
        # The same physical mutation remains legal ordinary C under Clang.
        control_obj = case/'clang.o'
        command(name+'-clang',[clang,'-std=c99','-I',ROOT/'include','-c',source,'-o',control_obj])
        assert control_obj.exists();control_obj.unlink()
    # Stable manifest failures must be the intended physical diagnostic, not
    # an unrelated parser failure that happens to return a nonzero exit.
    codes = [4103,4103,4103,4103,4103,4103,4112,4113,4101,4101,4107]
    for variant,code in enumerate(codes,1):
        name = f'carrier_failure_{variant}'
        diag,obj = out/(name+'.json'),out/(name+'.o')
        command(name,[compiler,'--overlay=physics-units','-I',ROOT/'include',f'-DFAIL_KIND={variant}',
            '--emit-diags-json',diag,'-c',ROOT/'tests/final/cases/15__units_frame_local_failures.c','-o',obj],1)
        assert not obj.exists(),name
        errors = [d for d in json.loads(diag.read_text())['diagnostics'] if d['severity_name']=='error']
        assert len(errors)==1 and errors[0]['code_id']==code,(name,errors)
        assert errors[0]['line']>0 and errors[0]['column']>0 and errors[0]['category_name']=='extension',errors
    # A dimensionally legal sign error must fail numerical parity, not be called
    # a dimensional rejection. Use the actual runtime fixture with a local header.
    case = out/'sign_error';case.mkdir()
    old,new = 'k->tr[3][0] = -coupling;','k->tr[3][0] = coupling;'
    assert base.count(old)==1
    (case/'frame_local.h').write_text(base.replace(old,new))
    source = case/'runtime.c'
    original_include = '#include "../../fixtures/solver_physics/frame_local.h"'
    text = RUNTIME.read_text();assert text.count(original_include)==1
    source.write_text(text.replace(original_include,'#include "frame_local.h"'))
    bad = case/'bad'
    command('sign-error-build',[compiler,'--overlay=physics-units','-I',ROOT/'include',source,'-lm','-o',bad])
    command('sign-error-runtime',[bad],1)
    assert 'coefficient mismatch' in (out/'sign-error-runtime.log').read_text()
    bad.unlink()
    raw = out/'raw-matrix'
    command('raw-matrix-unchecked',[compiler,'--overlay=physics-units','-DBAD_MATRIX_SLOT','-I',ROOT/'include',
        ROOT/'tests/fixtures/solver_physics/structure.c','-o',raw])
    assert raw.exists();raw.unlink()
    rows[-1]['classification'] = 'unchecked_control'
    for binary in binaries:binary.unlink()
    result = dict(status='PASS',commands=len(rows),required_physical_rejections=21,
        ordinary_c_mutation_controls=10,numerical_mutation_controls=1,oracle_failure_controls=3,
        unchecked_controls=1,parity=detail,compiler_sha256=hashlib.sha256(compiler.read_bytes()).hexdigest(),
        kernel_sha256=hashlib.sha256(KERNEL.read_bytes()).hexdigest(),results=rows)
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print('Frame local: 108 coefficients / 198 result components match Clang and rational oracle; '
          '21 required rejections; numerical sign control and raw unchecked control PASS')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compiler',required=True,type=Path);p.add_argument('--out',type=Path)
    a=p.parse_args()
    if a.out:
        a.out.mkdir(parents=True,exist_ok=False);check(a.compiler.resolve(),a.out.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='fisics-frame-local-') as tmp:check(a.compiler.resolve(),Path(tmp))


if __name__=='__main__':main()
