#!/usr/bin/env python3
"""Unit fixtures for statistics and experiment partitioning; these are not game results."""
import csv
import importlib.util
import json
import math
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

sys.dont_write_bytecode = True
SCRIPT = Path(__file__).resolve().parents[1] / 'tools' / 'analyze_balancing.py'
spec = importlib.util.spec_from_file_location('analyze_balancing', SCRIPT)
analysis = importlib.util.module_from_spec(spec)
spec.loader.exec_module(analysis)


def unit_row(preset='baseline', profile='beginner', seed=1001, peak=16):
    # Deliberately artificial, short values exercise calculations without masquerading as observations.
    row = {field: 0.0 for field in analysis.METRICS}
    row.update(preset=preset, profile=profile, seed=seed, max_seconds=600, max_rounds=20,
               round_reached=3, rounds_completed=2, survival_seconds=300,
               mean_round_seconds=50, shots_fired=10, projectile_hits=5, accuracy=0.5,
               damage_dealt=125, engines_lost=5, engines_recovered=2,
               weapon_diversity=3, peak_projectiles=peak, censored=False)
    return row


def write_rows(path, rows):
    with path.open('w', encoding='utf-8', newline='') as output:
        writer = csv.DictWriter(output, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)


class StatisticalTests(unittest.TestCase):
    def test_known_mean_standard_deviation_and_small_sample_interval(self):
        result = analysis.interval(list(range(1, 21)))
        self.assertEqual(result['n'], 20)
        self.assertAlmostEqual(result['mean'], 10.5)
        self.assertAlmostEqual(result['sd'], math.sqrt(35))
        # Published small-sample t critical rounded to three decimals: df19, 95% => 2.093.
        half_width = 2.093 * math.sqrt(35 / 20)
        self.assertAlmostEqual(result['ci95_low'], 10.5 - half_width)
        self.assertAlmostEqual(result['ci95_high'], 10.5 + half_width)
        self.assertEqual(result['minimum'], 1)
        self.assertEqual(result['maximum'], 20)

    def test_wilson_known_rate_and_extremes(self):
        middle = analysis.wilson(10, 20)
        self.assertAlmostEqual(middle['rate'], 0.5)
        self.assertAlmostEqual(middle['ci95_low'], 0.2992980081982123)
        self.assertAlmostEqual(middle['ci95_high'], 0.7007019918017877)
        zero = analysis.wilson(0, 20)
        self.assertAlmostEqual(zero['ci95_low'], 0)
        self.assertAlmostEqual(zero['ci95_high'], 0.16112515805281938)
        all_success = analysis.wilson(20, 20)
        self.assertAlmostEqual(all_success['ci95_low'], 0.8388748419471806)
        self.assertAlmostEqual(all_success['ci95_high'], 1)

    def test_missing_sample_does_not_invent_confidence(self):
        self.assertIsNone(analysis.interval([])['mean'])
        self.assertIsNone(analysis.interval([4])['ci95_low'])
        self.assertIsNone(analysis.wilson(0, 0)['rate'])

    def test_score_bounds_and_integrity_gate(self):
        rows = []
        for preset, peak in [('baseline', 16), ('valid', 256), ('over_capacity', 257)]:
            for profile, target in zip(analysis.PROFILES, (3, 6, 10)):
                for seed in (1001, 1002):
                    row = unit_row(preset, profile, seed, peak)
                    row['round_reached'] = target
                    rows.append(row)
        report, long_rows, paired = analysis.analyze(rows, max_projectiles=256)
        self.assertEqual(len(report['groups']), 9)
        self.assertGreater(len(long_rows), 0)
        self.assertGreater(len(paired), 0)
        for result in report['ranking']:
            self.assertGreaterEqual(result['score'], 0)
            self.assertLessEqual(result['score'], 100)
            if result['preset'] == 'over_capacity':
                self.assertFalse(result['integrity_pass'])
                self.assertFalse(result['eligible_for_recommendation'])
            elif result['preset'] == 'baseline':
                self.assertFalse(result['eligible_for_recommendation'])
            else:
                self.assertTrue(result['integrity_pass'])
                self.assertTrue(result['eligible_for_recommendation'])


class PartitionAndCommandTests(unittest.TestCase):
    def test_duplicate_runs_rejected(self):
        with tempfile.TemporaryDirectory(prefix='balancing-analysis-unit-') as directory:
            path = Path(directory) / 'unit_fixture.csv'
            write_rows(path, [unit_row(), unit_row()])
            with self.assertRaisesRegex(ValueError, 'Duplicate run'):
                analysis.read_rows([path])

    def test_calibration_and_heldout_cannot_be_mixed(self):
        with tempfile.TemporaryDirectory(prefix='balancing-analysis-unit-') as directory:
            path = Path(directory) / 'unit_fixture.csv'
            write_rows(path, [unit_row(seed=1001), unit_row(seed=9001)])
            with self.assertRaisesRegex(ValueError, 'Calibration and held-out'):
                analysis.read_rows([path])

    def test_different_horizons_cannot_be_merged(self):
        with tempfile.TemporaryDirectory(prefix='balancing-analysis-unit-') as directory:
            path = Path(directory) / 'unit_fixture.csv'
            different = unit_row(seed=1002)
            different['max_seconds'] = 180
            write_rows(path, [unit_row(), different])
            with self.assertRaisesRegex(ValueError, 'equal time/round horizons'):
                analysis.read_rows([path])

    def test_heldout_command_marks_results_evaluation_only(self):
        with tempfile.TemporaryDirectory(prefix='balancing-analysis-unit-') as directory:
            path = Path(directory) / 'unit_fixture.csv'
            destination = Path(directory) / 'summary'
            rows = [unit_row(preset, profile, 9001)
                    for preset in ('baseline', 'candidate') for profile in analysis.PROFILES]
            write_rows(path, rows)
            result = subprocess.run([sys.executable, str(SCRIPT), '--input', str(path),
                                     '--output-dir', str(destination)],
                                    capture_output=True, text=True,
                                    env={**os.environ, 'PYTHONDONTWRITEBYTECODE': '1'}, check=False)
            self.assertEqual(result.returncode, 0, result.stderr)
            summary = json.loads((destination / 'summary.json').read_text(encoding='utf-8'))
            self.assertEqual(summary['stage'], 'heldout_evaluation_only')
            self.assertFalse(summary['selection_allowed'])
            self.assertTrue(all(not r['eligible_for_recommendation'] for r in summary['ranking']))
            self.assertTrue((destination / 'summary.csv').exists())
            self.assertTrue((destination / 'paired_baseline.csv').exists())


if __name__ == '__main__':
    unittest.main()
