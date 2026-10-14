#!/usr/bin/env python3
"""Bounded repeated-header ledger benchmark. Outputs must use a fresh directory."""
import argparse, csv, hashlib, json, os, re, signal, statistics, subprocess, time
from pathlib import Path

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compiler', type=Path, required=True)
    p.add_argument('--baseline', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--repetitions', type=int, default=3, choices=[1, 2, 3])
    a = p.parse_args()
    out = a.out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    start_all = time.monotonic()
    rows = []
    identities = {label: hashlib.sha256(bin.read_bytes()).hexdigest() for label, bin in [('baseline', a.baseline), ('candidate', a.compiler)]}
    for declarations in [1024, 4096]:
        f = out / f'fixtures-{declarations}'
        f.mkdir()
        (f / 'shared.h').write_text(''.join((f'extern int probe_global_{i:04d};\n' for i in range(declarations))))
        for i in range(16):
            (f / f'unit{i:02d}.c').write_text('#include "shared.h"\n' + ('int probe_global_0000=0;\nint main(void){return 0;}\n' if i == 0 else f'int unit_{i:02d}(void){{return {i};}}\n'))
        (f / 'conflict.c').write_text('extern double probe_global_0000;\nint conflict_marker(void){return 0;}\n')
        for units in [4, 8, 16]:
            for label, compiler in [('baseline', a.baseline.resolve()), ('candidate', a.compiler.resolve())]:
                for repeat in range(a.repetitions):
                    remaining = 360 - (time.monotonic() - start_all)
                    if remaining < 30:
                        raise RuntimeError('six-minute total experiment budget exhausted')
                    d = out / f'{label}-{declarations}-{units}-{repeat}'
                    d.mkdir()
                    inputs = [f / f'unit{i:02d}.c' for i in range(units)]
                    cmd = [str(compiler), '--overlay=physics-units', *[str(x) for x in inputs], '-o', str(d / 'app')]
                    env = os.environ.copy()
                    env.update(FISICS_MAX_PROCS='1', FISICS_TIMEOUT_SEC='30', FISICS_PROFILE_MODE='timing', FISICS_PROFILE_STREAM='1', FISICS_PROFILE_PATH=str(d / 'timing.csv'))
                    before = set(Path('/private/tmp').glob('mycc-*.o'))
                    t = time.monotonic()
                    timeout = False
                    with (d / 'compile.log').open('w') as log:
                        proc = subprocess.Popen(['/usr/bin/time', '-l', *cmd], env=env, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
                        try:
                            rc = proc.wait(timeout=30)
                        except subprocess.TimeoutExpired:
                            timeout = True
                            os.killpg(proc.pid, signal.SIGKILL)
                            rc = proc.wait()
                    elapsed = time.monotonic() - t
                    phases = {}
                    timing = d / 'timing.csv'
                    if timing.exists():
                        for row in csv.DictReader(timing.open()):
                            if row['name'] in ['preprocess', 'parse', 'semantic', 'codegen']:
                                phases[row['name']] = phases.get(row['name'], 0) + float(row['duration_ms'])
                    log = (d / 'compile.log').read_text()
                    rss = re.search('(\\d+)\\s+maximum resident set size', log)
                    row = {'label': label, 'declarations': declarations, 'units': units, 'repeat': repeat, 'command': cmd, 'exit': rc, 'timeout': timeout, 'seconds': elapsed, 'phase_ms': phases, 'rss_bytes': int(rss[1]) if rss else None, 'new_tmp_objects_observed': [str(x) for x in set(Path('/private/tmp').glob('mycc-*.o')) - before]}
                    if not rc:
                        row['run_exit'] = subprocess.run([str(d / 'app')], timeout=3).returncode
                    rows.append(row)
                    (out / 'results.json').write_text(json.dumps({'binary_sha256': identities, 'runs': rows}, indent=2))
                    print(label, declarations, units, repeat, rc, round(elapsed, 3), flush=True)
                    if not rc and row['run_exit'] != 0:
                        raise RuntimeError('compiled performance case returned nonzero')
                    if rc:
                        if label == 'candidate':
                            raise RuntimeError('candidate performance case failed')
                        break
    f = out / 'fixtures-4096'
    cmd = [str(a.compiler.resolve()), '--overlay=physics-units', *[str(f / f'unit{i:02d}.c') for i in range(16)], str(f / 'conflict.c'), '-o', str(out / 'must-not-exist')]
    q = subprocess.run(cmd, capture_output=True, text=True, timeout=min(30, max(1, 360 - (time.monotonic() - start_all))))
    (out / 'late-conflict.log').write_text(q.stdout + q.stderr)
    assert q.returncode == 1 and 'Conflicting types for variable' in q.stdout + q.stderr and (not (out / 'must-not-exist').exists())
    sizes = [n for n in [4, 8, 16] if all((x['exit'] == 0 for x in rows if x['label'] == 'baseline' and x['declarations'] == 4096 and (x['units'] == n)))]
    n = max(sizes)
    med = {label: statistics.median((x['seconds'] for x in rows if x['label'] == label and x['declarations'] == 4096 and (x['units'] == n))) for label in identities}
    ratio = med['baseline'] / med['candidate']
    summary = {'status': 'PASS' if ratio >= 2 else 'REWORK', 'largest_baseline_size': n, 'median_seconds': med, 'speedup': ratio, 'late_conflict_exit': q.returncode, 'total_seconds': time.monotonic() - start_all}
    (out / 'summary.json').write_text(json.dumps(summary, indent=2))
    (out / 'inputs.json').write_text(json.dumps({str(f.relative_to(out)): hashlib.sha256(f.read_bytes()).hexdigest() for f in out.glob('fixtures-*/*')}, indent=2))
    print(summary)
    return 0 if ratio >= 2 else 1
if __name__ == '__main__':
    raise SystemExit(main())
