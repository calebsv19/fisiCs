#!/usr/bin/env python3
"""Measure matched overlay compilation and separate selected strict/report cost on macOS."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import statistics
import subprocess

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def check(compiler,manifest,budget,out):
    if platform.system()!='Darwin':raise RuntimeError('This runner parses macOS /usr/bin/time -l; other hosts need their own resource parser.')
    if os.environ.get('FISICS_OVERLAY') or os.environ.get('DISABLE_CODEGEN'):
        raise RuntimeError('Unset FISICS_OVERLAY and DISABLE_CODEGEN: compilation must be controlled by this runner.')
    spec=json.loads(manifest.read_text());limits=json.loads(budget.read_text())
    samples=limits['samples'];assert samples>=5
    physical=spec.get('physical_policy')
    modes=['base','units','physical'] if physical else ['base','units']
    overlay_args={'base':[],'units':['--overlay=physics-units'],'physical':['--overlay=physics-semantics','--physical-semantics-policy',str(physical)]}
    env={**os.environ,'FISICS_MAX_PROCS':'0','PYTHONDONTWRITEBYTECODE':'1','TMPDIR':str(out/'tmp')}
    (out/'tmp').mkdir();rows=[];result=dict(schema='fisics.overlay_cost.v2',passed=False,budget=limits,
        runner_sha256=sha(__file__),overlay_environment=None,manifest_sha256=sha(manifest),budget_sha256=sha(budget),compiler_sha256=sha(compiler),host=dict(platform=platform.platform(),machine=platform.machine(),loadavg=os.getloadavg()),samples=rows,summary=[])
    cpu=subprocess.run(['sysctl','-n','machdep.cpu.brand_string'],capture_output=True,text=True)
    result['host']['cpu']=cpu.stdout.strip()
    inputs={p:sha(p) for p in spec['dependencies']}
    result['source_sha256']=inputs
    result['preprocessing']='one clang -fno-blocks -E -P -D__FISICS__=1 expansion per TU; attributes erased into whitespace: base all dim/unit/kind/origin; units kind/origin; physical retains all; preprocessing preparation excluded from sample times'
    result['prepared_inputs']=[]
    def measure(label,args,warmup=False):
        timing=out/(label+'.time');log=out/(label+'.log')
        cmd=[str(compiler),*map(str,args)]
        with log.open('w') as handle:
            p=subprocess.run(['/usr/bin/time','-l','-o',str(timing),*cmd],stdout=handle,stderr=subprocess.STDOUT,env=env,timeout=60)
        assert p.returncode==0,(label,p.returncode)
        assert 'Error:' not in log.read_text(),(label,'backend diagnostic')
        raw=timing.read_text();wall=re.search(r'([\d.]+)\s+real',raw);rss=re.search(r'(\d+)\s+maximum resident set size',raw)
        assert wall and rss,(label,raw)
        item=dict(label=label,command=cmd,seconds=float(wall[1]),peak_rss_bytes=int(rss[1]),warmup=warmup)
        rows.append(item);return item
    def median(values,key):return statistics.median(v[key] for v in values)
    try:
        for item in spec['translation_units']:
            group={mode:[] for mode in modes};name=item['id'];args=item['args']
            pp_command=['clang','-std=c11','-fno-blocks','-E','-P','-D__FISICS__=1',*args,item['source']]
            pp=subprocess.run(pp_command,capture_output=True,text=True,env=env,timeout=60)
            assert pp.returncode==0,(name,pp.stderr)
            def erase(text,names):
                pattern=r'\[\[\s*fisics::(?:'+names+r')\s*\(.*?\)\s*\]\]'
                return re.subn(pattern,lambda m: ''.join('\n' if c=='\n' else ' ' for c in m[0]),text,flags=re.DOTALL)
            plain,count=erase(pp.stdout,'dim|unit|quantity_kind|physical_origin|representation|physical_operation|frame|component_axis|applied_at|between_points|moment_reference|point_id|frame_transform|energy_form|energy_transfer')
            unit_text,kind_count=erase(pp.stdout,'quantity_kind|physical_origin|representation|physical_operation|frame|component_axis|applied_at|between_points|moment_reference|point_id|frame_transform|energy_form|energy_transfer')
            assert count>0 and (not physical or kind_count>0)
            prepared={};hashes={}
            for mode,text in [('base',plain),('units',unit_text),('physical',pp.stdout)]:
                if mode not in modes:continue
                path=out/(name+'-'+mode+'.c');path.write_text(text);prepared[mode]=path;hashes[mode]=sha(path)
                assert erase(text,'dim|unit|quantity_kind|physical_origin|representation|physical_operation|frame|component_axis|applied_at|between_points|moment_reference|point_id|frame_transform|energy_form|energy_transfer')[0]==plain
            result['prepared_inputs'].append(dict(id=name,command=pp_command,sha256=hashes,erased_attributes=count,semantic_attributes=kind_count,numerical_source_identical_outside_attributes=True))
            # Rotate positions; nine three-mode trials put each mode first three times.
            for trial in range(-1,samples):
                order=modes[trial%len(modes):]+modes[:trial%len(modes)]
                for mode in order:
                    label=f'{name}-{trial+1}-{mode}';obj=out/(label+'.o')
                    row=measure(label,[*overlay_args[mode],*args,'-c',prepared[mode],'-o',obj],trial<0)
                    obj.unlink()
                    if trial>=0:group[mode].append(row)
            for baseline,overlay in ([('base','units'),('base','physical'),('units','physical')] if physical else [('base','units')]):
                a,b=group[baseline],group[overlay];aw=median(a,'seconds');bw=median(b,'seconds');ar=median(a,'peak_rss_bytes');br=median(b,'peak_rss_bytes')
                accepted=bw<=limits['wall_ratio']*aw+limits['wall_allowance_seconds'] and br<=limits['rss_ratio']*ar+limits['rss_allowance_bytes']
                result['summary'].append(dict(id=name,kind='compile',baseline=baseline,overlay=overlay,base_median_seconds=aw,overlay_median_seconds=bw,
                    wall_ratio=bw/aw if aw else None,base_median_rss_bytes=ar,overlay_median_rss_bytes=br,rss_ratio=br/ar,
                    base_seconds_range=[min(r['seconds'] for r in a),max(r['seconds'] for r in a)],overlay_seconds_range=[min(r['seconds'] for r in b),max(r['seconds'] for r in b)],accepted=accepted))
        for item in spec['strict_functions']:
            values=[]
            for trial in range(-1,samples):
                label=f"{item['id']}-strict-{trial+1}";report=out/(label+'.json')
                physical_report=item.get('physical',False)
                permissions=item.get('permissions',['literal-assumptions','declared-boundaries'])
                assert len(set(permissions))==len(permissions) and set(permissions)<={'literal-assumptions','declared-boundaries'}
                trust_flags=['--physical-semantics-allow-'+p for p in permissions]
                flags=(['--physical-semantics-function',item['function'],'--emit-physical-semantics-json',report,
                    *(['--physical-semantics-strict',*trust_flags] if item.get('accepted',True) else [])] if physical_report else
                    ['--units-coverage-strict','--units-coverage-allow-literal-assumptions','--units-coverage-allow-declared-boundaries','--units-coverage-function',item['function'],'--emit-units-coverage-json',report])
                row=measure(label,[*overlay_args['physical' if physical_report else 'units'],*item['args'],*flags,item['source']],trial<0)
                data=json.loads(report.read_text());policy=data['selection' if physical_report else 'strict_policy']
                assert policy['accepted']==item.get('accepted',True)
                if policy['accepted']:assert policy['blocked']==0
                for key in ['checked','permitted_trust']:
                    if 'expected_'+key in item:assert policy[key]==item['expected_'+key],(item['id'],key,policy)
                if not policy['accepted']:assert policy['blocked']>0
                if trial>=0:values.append(row)
            seconds=median(values,'seconds');rss=median(values,'peak_rss_bytes')
            result['summary'].append(dict(id=item['id'],kind='physical_strict' if item.get('physical') and item.get('accepted',True) else 'physical_report' if item.get('physical') else 'units_strict',selected_body_accepted=item.get('accepted',True),selection_counts=dict(total=policy.get('total',policy['required']+policy['ordinary_controls']),**{k:policy[k] for k in ['required','ordinary_controls','checked','permitted_trust','blocked']}),median_seconds=seconds,median_rss_bytes=rss,
                max_seconds=max(v['seconds'] for v in values),max_rss_bytes=max(v['peak_rss_bytes'] for v in values),
                accepted=max(v['seconds'] for v in values)<=limits['strict_seconds'] and max(v['peak_rss_bytes'] for v in values)<=limits['strict_rss_bytes']))
        assert all(sha(p)==v for p,v in inputs.items()),'source drift while measuring'
        assert sha(compiler)==result['compiler_sha256'],'compiler drift while measuring'
        result['passed']=all(s['accepted'] for s in result['summary'])
        assert result['passed'],'cost budget exceeded (see results.json)'
        print(f"Overlay cost: PASS; {len(result['summary'])} workloads, {samples} samples/mode plus warmups")
    finally:(out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ['compiler','manifest','budget','out']:p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();a.out.mkdir(parents=True,exist_ok=False)
    check(a.compiler.resolve(),a.manifest.resolve(),a.budget.resolve(),a.out.resolve())
if __name__=='__main__':main()
