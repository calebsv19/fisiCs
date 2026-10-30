#!/usr/bin/env python3
"""Read-only accepted-prefix diagnostics for a frozen Thermal electrical-heat bundle."""
import argparse
import json
from pathlib import Path
import sys
from electrothermal_binding import load, require
from electrothermal_diagnostics import inspect_bundle, publish, verify_report


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ['host-root','native','bundle']:
        p.add_argument('--'+name,type=Path,required=True)
    group = p.add_mutually_exclusive_group(required=True)
    group.add_argument('--output',type=Path,help='New JSON file; never overwrites or writes inside the input bundle')
    group.add_argument('--verify-report',type=Path,help='Check a saved report against its frozen bundle and native observations')
    p.add_argument('--through',type=int,help='Exact stored prefix, including zero; defaults to all available accepted samples')
    a = p.parse_args(); sys.dont_write_bytecode = True
    sys.path.insert(0,str(a.host_root.resolve()/'tools'))
    import electrical_heat as heat
    if a.verify_report:
        require(a.through is None, '--through is only for new reports')
        value = verify_report(load(a.verify_report),a.bundle,a.native,heat)
    else:
        value = inspect_bundle(a.bundle,a.native,heat,a.through)
        publish(value,a.output,a.bundle)
    row = value['samples'][-1]
    print(json.dumps(dict(passed=True,available_completed_steps=value['available_completed_steps'],
                          reported_through_step=value['reported_through_step'],samples=len(value['samples']),
                          accepted_j=row['accepted_allocated_j'],pending_at_sample_j=row['pending_allocated_at_sample_j'],
                          maximum_storage_residual_j=value['maxima']['receipt_storage_residual_abs_j'])))


if __name__ == '__main__':
    try:
        main()
    except (ValueError,OSError,KeyError,TypeError) as error:
        sys.exit('electrothermal diagnostics refused: '+str(error))
