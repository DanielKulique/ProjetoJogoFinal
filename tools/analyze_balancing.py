#!/usr/bin/env python3
"""Summarize real headless runs; no third-party Python dependencies.

Means of survival are restricted to the simulation horizon, not complete lifetimes.
Selection score is the engineering preference declared in EXPERIMENT_PROTOCOL.md.
"""
import argparse
import csv
import json
import math
import statistics
from collections import defaultdict
from pathlib import Path

T95 = [0, 12.706, 4.303, 3.182, 2.776, 2.571, 2.447, 2.365, 2.306,
       2.262, 2.228, 2.201, 2.179, 2.160, 2.145, 2.131, 2.120,
       2.110, 2.101, 2.093, 2.086, 2.080, 2.074, 2.069, 2.064,
       2.060, 2.056, 2.052, 2.048, 2.045, 2.042]
PROFILES = ('beginner', 'intermediate', 'advanced')
METRICS = ('round_reached', 'rounds_completed', 'survival_seconds',
           'mean_round_seconds', 'enemy_kills', 'player_kills', 'shots_fired',
           'projectile_hits', 'accuracy', 'damage_dealt', 'asteroid_damage_dealt',
           'damage_received', 'engines_lost', 'engines_recovered',
           'upgrades_collected', 'upgrade_rate_per_minute', 'weapon_diversity',
           'peak_projectiles', 'mean_projectiles', 'wall_seconds')
PATTERNS = ('standard', 'burst', 'heavy', 'circle', 'triangle', 'square', 'spread', 'parallel')


def interval(values):
    n = len(values)
    if not n:
        return dict(n=0, mean=None, sd=None, ci95_low=None, ci95_high=None, minimum=None, maximum=None)
    mean = statistics.mean(values)
    sd = statistics.stdev(values) if n > 1 else 0.0
    # Table for small samples; interpolation uses conservative lower df for larger samples.
    df = n - 1
    critical = T95[df] if 0 < df <= 30 else (2.042 if df < 60 else 2.0 if df < 120 else 1.98 if df < 1000 else 1.96)
    error = critical * sd / math.sqrt(n) if n > 1 else None
    return dict(n=n, mean=mean, sd=sd,
                ci95_low=mean-error if error is not None else None,
                ci95_high=mean+error if error is not None else None,
                minimum=min(values), maximum=max(values))


def wilson(successes, n):
    if not n:
        return dict(n=0, successes=successes, rate=None, ci95_low=None, ci95_high=None)
    p, z = successes / n, 1.959963984540054
    den = 1 + z*z/n
    center = (p + z*z/(2*n)) / den
    half = z*math.sqrt(p*(1-p)/n + z*z/(4*n*n)) / den
    return dict(n=n, successes=successes, rate=p, ci95_low=center-half, ci95_high=center+half)


def read_rows(paths):
    rows, seen = [], set()
    for path in paths:
        with Path(path).open(newline='', encoding='utf-8') as source:
            for raw in csv.DictReader(source):
                row = dict(raw)
                for field in METRICS + tuple(f'{p}_{s}' for p in PATTERNS for s in ('shots', 'hits', 'damage', 'accuracy', 'damage_per_shot')):
                    if field in raw and raw[field] != '':
                        row[field] = float(raw[field])
                        if not math.isfinite(row[field]):
                            raise ValueError(f'Nonfinite {field} in {path}: {raw}')
                row['seed'] = int(raw['seed'])
                row['censored'] = str(raw['censored']).lower() in ('1', 'true', 'yes')
                key = (row['preset'], row['profile'], row['seed'])
                if key in seen:
                    raise ValueError(f'Duplicate run {key}; do not mix exploratory and expanded calibration rows')
                seen.add(key)
                rows.append(row)
    if not rows:
        raise ValueError('No simulation rows')
    if any(1001 <= r['seed'] <= 1020 for r in rows) and any(9001 <= r['seed'] <= 9020 for r in rows):
        raise ValueError('Calibration and held-out seeds must be analyzed separately')
    horizons = {(r.get('max_seconds'), r.get('max_rounds')) for r in rows}
    if len(horizons) != 1:
        raise ValueError('Compare only equal time/round horizons')
    return rows


def quality(value, target, width=None):
    return max(0.0, 1 - abs(value-target)/(target if width is None else width))


def analyze(rows, max_projectiles=256, baseline='baseline'):
    grouped = defaultdict(list)
    for row in rows:
        grouped[row['preset'], row['profile']].append(row)
    summaries = {}
    long_rows = []
    for (preset, profile), group in sorted(grouped.items()):
        numeric = [m for m in METRICS if m in group[0]]
        numeric += [f'{p}_{s}' for p in PATTERNS for s in ('shots', 'hits', 'damage', 'accuracy', 'damage_per_shot') if f'{p}_{s}' in group[0]]
        metrics = {m: interval([r[m] for r in group]) for m in numeric}
        for metric, summary in metrics.items():
            long_rows.append(dict(preset=preset, profile=profile, metric=metric, **summary))
        outcomes = {
            'defeat': wilson(sum(not r['censored'] for r in group), len(group)),
            'round2': wilson(sum(r['round_reached'] >= 2 for r in group), len(group)),
            'early_defeat60': wilson(sum(not r['censored'] and r['survival_seconds'] < 60 for r in group), len(group)),
        }
        shots = sum(r['shots_fired'] for r in group)
        losses = sum(r['engines_lost'] for r in group)
        metrics['pooled_accuracy'] = sum(r['projectile_hits'] for r in group) / shots if shots else 0
        metrics['pooled_repair_fraction'] = sum(r['engines_recovered'] for r in group) / losses if losses else 0
        summaries[f'{preset}/{profile}'] = dict(n=len(group), metrics=metrics, outcomes=outcomes)
    scores = []
    for preset in sorted({r['preset'] for r in rows}):
        if any((preset, p) not in grouped for p in PROFILES):
            continue
        groups = [grouped[preset, p] for p in PROFILES]
        means = [statistics.mean(r['round_reached'] for r in g) for g in groups]
        deaths = [sum(not r['censored'] for r in g)/len(g) for g in groups]
        progress = statistics.mean(quality(r, t) for r, t in zip(means, (3, 6, 10)))
        risk = statistics.mean(quality(p, t, 1) for p, t in zip(deaths, (.65, .45, .25)))
        skill = quality(means[2]-means[0], 7)
        recovery, diversity, tempo = [], [], []
        for group in groups:
            losses = sum(r['engines_lost'] for r in group)
            recovered = sum(r['engines_recovered'] for r in group)
            recovery.append(quality(recovered/losses, .4, .6) if losses else 0)
            diversity.append(min(1.0, statistics.mean(r['weapon_diversity'] for r in group)/3))
            completed = [r for r in group if r['rounds_completed'] > 0]
            mean_round = (sum(r['mean_round_seconds']*r['rounds_completed'] for r in completed) /
                          sum(r['rounds_completed'] for r in completed)) if completed else None
            tempo.append(quality(mean_round, 50, 100) if mean_round is not None else 0)
        parts = dict(progress=progress, risk=risk, skill=skill,
                     recovery=statistics.mean(recovery), diversity=statistics.mean(diversity), tempo=statistics.mean(tempo))
        score = 100 * sum(parts[k]*w for k, w in dict(progress=.4, risk=.2, skill=.15, recovery=.1, diversity=.1, tempo=.05).items())
        peak = max(r['peak_projectiles'] for g in groups for r in g)
        integrity = peak <= max_projectiles
        scores.append(dict(preset=preset, score=score, components=parts,
                           integrity_pass=integrity, eligible_for_recommendation=integrity and preset != baseline,
                           skill_ordered=means[0] <= means[1] <= means[2],
                           round_means=dict(zip(PROFILES, means)), peak_projectiles=peak))
    scores.sort(key=lambda r: r['score'], reverse=True)
    lookup = {(r['preset'], r['profile'], r['seed']): r for r in rows}
    paired = []
    for preset in sorted({r['preset'] for r in rows} - {baseline}):
        for profile in PROFILES:
            comparisons = [(r, lookup.get((baseline, profile, r['seed']))) for r in grouped.get((preset, profile), [])]
            comparisons = [(a, b) for a, b in comparisons if b is not None]
            for metric in ('round_reached', 'survival_seconds', 'enemy_kills', 'accuracy', 'damage_dealt', 'engines_lost', 'engines_recovered', 'upgrades_collected', 'weapon_diversity'):
                paired.append(dict(preset=preset, baseline=baseline, profile=profile, metric=metric,
                                   **interval([a[metric]-b[metric] for a, b in comparisons])))
    return dict(groups=summaries, ranking=scores, paired_baseline=paired), long_rows, paired


def write_csv(path, rows):
    if not rows:
        path.write_text('', encoding='utf-8')
        return
    with path.open('w', newline='', encoding='utf-8') as output:
        writer = csv.DictWriter(output, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', nargs='+', required=True)
    parser.add_argument('--output-dir', required=True)
    parser.add_argument('--baseline', default='baseline')
    parser.add_argument('--max-projectiles', type=int, default=256)
    args = parser.parse_args()
    rows = read_rows(args.input)
    report, summaries, paired = analyze(rows, args.max_projectiles, args.baseline)
    heldout = all(9001 <= r['seed'] <= 9020 for r in rows)
    report['selection_allowed'] = not heldout
    report['stage'] = 'heldout_evaluation_only' if heldout else 'calibration_or_exploration'
    if heldout:
        for score in report['ranking']:
            score['eligible_for_recommendation'] = False
    report['inputs'] = args.input
    report['run_count'] = len(rows)
    report['seed_range'] = [min(r['seed'] for r in rows), max(r['seed'] for r in rows)]
    report['interpretation'] = 'Offline engineering proxies only; no psychological Flow proof. Survival is right-censored at the horizon.'
    destination = Path(args.output_dir)
    destination.mkdir(parents=True, exist_ok=True)
    (destination/'summary.json').write_text(json.dumps(report, indent=2, allow_nan=False)+'\n', encoding='utf-8')
    write_csv(destination/'summary.csv', summaries)
    write_csv(destination/'paired_baseline.csv', paired)
    for result in report['ranking']:
        print(f"{result['preset']}: score={result['score']:.3f}, integrity={result['integrity_pass']}, skill_ordered={result['skill_ordered']}")


if __name__ == '__main__':
    main()
