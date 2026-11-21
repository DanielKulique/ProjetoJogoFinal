#!/usr/bin/env python3
"""Reproduce the fixed offline protocol; heldout starts only after selection is saved.

Requires Python standard library, built space_arena_sim, and ~696 real Game runs.
Wall-clock timing is intentionally excluded from gameplay reproduction checks.
"""
import argparse
import csv
import hashlib
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PRESETS = 'baseline,procedural_baseline,repair,balanced,gentle,pressure,variety,environment'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--simulator', default='build/release/space_arena_sim')
    parser.add_argument('--output-dir', default='build/balancing-reproduction')
    parser.add_argument('--verify-against', help='Compare gameplay CSVs to an earlier protocol run')
    args = parser.parse_args()
    simulator = str(Path(args.simulator).resolve())
    output = Path(args.output_dir).resolve()
    if args.verify_against and output == Path(args.verify_against).resolve():
        parser.error('Reproduction output must differ from the archived verification directory')
    output.mkdir(parents=True, exist_ok=True)
    commands = []

    def execute(command):
        commands.append(command)
        subprocess.run(command, check=True, cwd=ROOT)

    def simulate(presets, seeds, runs, result, configs, extra=()):
        execute([simulator, '--preset', presets, '--profile', 'all', '--seed-start', str(seeds),
                 '--runs', str(runs), '--seconds', '600', '--max-rounds', '20',
                 '--output', str(result), '--configs-output', str(configs), *extra])

    def analyze(inputs, directory):
        execute([sys.executable, str(ROOT/'tools/analyze_balancing.py'), '--input',
                 *(str(path) for path in inputs), '--output-dir', str(directory)])
        return json.loads((directory/'summary.json').read_text())

    simulate(PRESETS, 1001, 6, output/'exploration.csv', output/'exploration_configs.csv')
    exploration = analyze([output/'exploration.csv'], output/'exploration_summary')
    best = [r['preset'] for r in exploration['ranking'] if r['eligible_for_recommendation']][:3]
    simulate(','.join(['baseline', *best]), 1001, 20, output/'calibration.csv', output/'calibration_configs.csv')
    analyze([output/'calibration.csv'], output/'calibration_summary')
    execute([sys.executable, str(ROOT/'tools/run_refinement.py'), '--simulator', simulator,
             '--output-dir', str(output/'refinement_exploration')])
    refinements = analyze([output/'refinement_exploration/runs.csv'], output/'refinement_exploration/summary')
    refined = [r['preset'] for r in refinements['ranking'] if r['eligible_for_recommendation']][:2]
    execute([sys.executable, str(ROOT/'tools/run_refinement.py'), '--simulator', simulator,
             '--labels', ','.join(refined), '--runs', '20', '--output-dir', str(output/'refinement_calibration')])
    calibration_paths = [output/'calibration.csv', output/'refinement_calibration/runs.csv']
    final = analyze(calibration_paths, output/'final_calibration_summary')
    selected = next(r for r in final['ranking'] if r['eligible_for_recommendation'])
    from run_refinement import CANDIDATES
    choice = selected['preset']
    selected_preset = 'pressure' if choice in CANDIDATES else choice
    selected_args = ['--label', choice, *CANDIDATES[choice]] if choice in CANDIDATES else []
    # This file is written BEFORE any heldout process starts. Do not select by heldout scores.
    selection = dict(selected_preset=choice, selection_stage='calibration_only', score=selected['score'],
                     score_components=selected['components'], calibration_seeds=[1001, 1020],
                     heldout_seeds_not_used_for_selection=[9001, 9020], frozen_before_heldout=True,
                     selected_cli=[selected_preset, *selected_args],
                     data_sha256={str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in calibration_paths})
    (output/'selection.json').write_text(json.dumps(selection, indent=2)+'\n')
    simulate('baseline', 9001, 20, output/'heldout_baseline.csv', output/'heldout_baseline_configs.csv')
    simulate(selected_preset, 9001, 20, output/'heldout_selected.csv', output/'heldout_selected_configs.csv', selected_args)
    analyze([output/'heldout_baseline.csv', output/'heldout_selected.csv'], output/'heldout_summary')
    (output/'commands.json').write_text(json.dumps(commands, indent=2)+'\n')
    if args.verify_against:
        original = Path(args.verify_against).resolve()
        paths = ['exploration.csv', 'calibration.csv', 'refinement_exploration/runs.csv',
                 'refinement_calibration/runs.csv', 'heldout_baseline.csv', 'heldout_selected.csv']
        def gameplay(path):
            with path.open(newline='') as source:
                rows = [{key: value for key, value in row.items() if key != 'wall_seconds'} for row in csv.DictReader(source)]
            return sorted(rows, key=lambda row: (row['preset'], row['profile'], int(row['seed'])))
        for path in paths:
            if gameplay(output/path) != gameplay(original/path):
                raise RuntimeError('Gameplay reproduction mismatch: '+path)
        if choice != json.loads((original/'selection.json').read_text())['selected_preset']:
            raise RuntimeError('Calibration recommendation changed')
        print('All 696 gameplay rows reproduced exactly; wall_seconds excluded.')


if __name__ == '__main__':
    main()
