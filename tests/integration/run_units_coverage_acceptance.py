#!/usr/bin/env python3
"""Finite joined P3 acceptance matrix. Reports retain required failures as failures."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
ROOT=Path(__file__).resolve().parents[2]
MANIFEST=ROOT/'tests/fixtures/solver_physics/coverage_acceptance.json'

def check(compiler,out):
    manifest=json.loads(MANIFEST.read_text());assert manifest['schema']=='fisiCs.units_coverage_acceptance.v1'
    commands=[];reports={};matrix=[]
    def run(label,cmd,code=0):
        p=subprocess.run([str(x) for x in cmd],cwd=ROOT,env={**os.environ,'FISICS_MAX_PROCS':'0'},capture_output=True,text=True,timeout=180)
        (out/(label+'.log')).write_text(p.stdout+p.stderr)
        commands.append(dict(id=label,command=[str(x) for x in cmd],expected_exit=code,exit=p.returncode))
        assert p.returncode==code,(label,p.returncode,p.stdout,p.stderr)
    for case in manifest['cases']:
        label=case['id'];dest=out/(label+'.json');txt=out/(label+'.txt')
        cmd=[compiler,'--overlay=physics-units','--units-coverage-strict','--units-coverage-function',case['function'],
             '--emit-units-coverage-json',dest,'--emit-units-coverage-text',txt]
        if case.get('literal_permission'):cmd+=['--units-coverage-allow-literal-assumptions']
        # Both trust permissions must fail to waive unknown/unsupported/mismatched inputs.
        if case['group'] in ('metadata_removal','unsupported_boundary') or case.get('contradictory'):
            cmd+=['--units-coverage-allow-literal-assumptions','--units-coverage-allow-declared-boundaries']
        cmd+=['-D'+x for x in case.get('defines',[])]+[ROOT/manifest['source']]
        run(label,cmd,case['exit']);d=json.loads(dest.read_text());p=d['strict_policy'];den=d['denominator']
        assert p['contract']==manifest['policy'] and p['accepted'] is (case['exit']==0)
        assert den['total']==len(d['obligations'])==p['required']+p['ordinary_controls']
        assert p['required']==p['checked']+p['permitted_trust']+p['blocked']
        assert p['ordinary_controls']==sum(not row['strict_required'] for row in d['obligations'])
        if case['exit']==0:
            assert d['scope']['enumeration_complete'] and p['checked']>0 and p['blocked']==0
        if 'controls' in case:assert p['ordinary_controls']==case['controls']
        if 'trust' in case:assert p['permitted_trust']==case['trust']
        if case.get('blocked'):assert p['blocked']>0
        if case.get('incomplete'):assert not d['scope']['enumeration_complete']
        if case.get('contradictory'):assert den['contradictory']>0
        if case.get('stable_total'):
            parent=reports[case['paired_with']]
            assert den['total']==parent['denominator']['total'] and p['ordinary_controls']==parent['strict_policy']['ordinary_controls']
            missing=[row for row in d['obligations'] if row['strict_required'] and row['state']=='unchecked']
            assert missing and any(row['loss_location'] for row in missing)
        reports[label]=d
        matrix.append(dict(id=label,group=case['group'],accepted=p['accepted'],enumeration_complete=d['scope']['enumeration_complete'],observations=den,policy=p))
    run('clang-source',['clang','-std=c99','-pedantic-errors','-I',ROOT/'include','-c',ROOT/manifest['source'],'-o',out/'control.o'])
    (out/'control.o').unlink()
    suite_commands=0
    for script in manifest['prerequisite_suites']:
        label=Path(script).stem;suite=out/label
        run(label,['python3','-B',ROOT/'tests/integration'/script,'--compiler',compiler,'--out',suite])
        results=json.loads((suite/'results.json').read_text());assert results['passed'];suite_commands+=len(results['commands'])
    result=dict(schema=manifest['schema'],passed=True,compiler_sha256=hashlib.sha256(compiler.read_bytes()).hexdigest(),
        manifest_sha256=hashlib.sha256(MANIFEST.read_bytes()).hexdigest(),source_sha256=hashlib.sha256((ROOT/manifest['source']).read_bytes()).hexdigest(),
        direct_commands=commands,prerequisite_commands=suite_commands,acceptance=matrix,scope='selected AST compatibility obligations; no application adoption or runtime physics claim')
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(f'P3 acceptance: PASS ({len(matrix)} fixed cases; {len(commands)} direct commands; {suite_commands} prerequisite commands)')

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
    a=p.parse_args();a.out.mkdir(parents=True,exist_ok=False);check(a.compiler.resolve(),a.out.resolve())
if __name__=='__main__':main()
