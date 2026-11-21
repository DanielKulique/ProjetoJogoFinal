#!/usr/bin/env python3
"""Run the four documented training-only refinements through the real simulator."""
import argparse
import csv
import json
import subprocess
from pathlib import Path

CANDIDATES = {
    'growth14': ['--drop', '.90', '--engine-chance', '.50', '--advanced-chance', '.30', '--health-growth', '14', '--spawn-interval', '1.6', '--pickup-lifetime', '35'],
    'growth18': ['--drop', '.90', '--engine-chance', '.60', '--advanced-chance', '.30', '--health-growth', '18', '--spawn-interval', '1.6', '--pickup-lifetime', '35'],
    'paced': ['--drop', '.90', '--engine-chance', '.60', '--advanced-chance', '.30', '--health-growth', '14', '--spawn-interval', '1.3', '--pickup-lifetime', '35'],
    'recovery_long': ['--drop', '.95', '--engine-chance', '.60', '--advanced-chance', '.35', '--health-growth', '10', '--spawn-interval', '1.6', '--pickup-lifetime', '45'],
}


def merge(paths, destination):
    fields, rows = None, []
    for path in paths:
        with path.open(newline='', encoding='utf-8') as source:
            reader = csv.DictReader(source)
            if fields is not None and fields != reader.fieldnames:
                raise ValueError('Incompatible CSV schemas')
            fields = reader.fieldnames
            rows.extend(reader)
    with destination.open('w', newline='', encoding='utf-8') as output:
        writer = csv.DictWriter(output, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--simulator', default='build/release/space_arena_sim')
    parser.add_argument('--labels', default=','.join(CANDIDATES))
    parser.add_argument('--seed-start', type=int, default=1001)
    parser.add_argument('--runs', type=int, default=6)
    parser.add_argument('--output-dir', required=True)
    args = parser.parse_args()
    if args.seed_start < 1001 or args.seed_start + args.runs - 1 > 1020 or args.runs <= 0:
        parser.error('Refinement is restricted to calibration seeds 1001..1020')
    labels = args.labels.split(',')
    if not labels or any(label not in CANDIDATES for label in labels) or len(set(labels)) != len(labels):
        parser.error('Unknown or repeated refinement label')
    destination = Path(args.output_dir)
    destination.mkdir(parents=True, exist_ok=True)
    commands, results, configs = [], [], []
    for label in labels:
        result = destination / (label + '.csv')
        config = destination / (label + '_configs.csv')
        command = [args.simulator, '--preset', 'pressure', '--label', label, '--profile', 'all',
                   '--seed-start', str(args.seed_start), '--runs', str(args.runs), '--seconds', '600',
                   '--max-rounds', '20', '--output', str(result), '--configs-output', str(config),
                   *CANDIDATES[label]]
        subprocess.run(command, check=True)
        commands.append(command)
        results.append(result)
        configs.append(config)
    merge(results, destination / 'runs.csv')
    merge(configs, destination / 'configs.csv')
    (destination / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
