#!/usr/bin/env python3
"""Validate simulator metadata and protect experiment CSVs from path aliases."""
import csv
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    if len(sys.argv) != 2:
        raise ValueError('Expected simulator executable path')
    executable = str(Path(sys.argv[1]).resolve())
    environment = dict(os.environ)
    environment.pop('DISPLAY', None)
    common = ['--runs', '1', '--seconds', '.01']
    with tempfile.TemporaryDirectory(prefix='space-arena-sim-cli-') as temporary:
        directory = Path(temporary)
        result_file = directory / 'results.csv'
        config_file = directory / 'configs.csv'
        accepted = ['--preset', 'balanced', '--profile', 'beginner', '--label', 'balanced_ttl35',
                    '--pickup-lifetime', '35', '--output', str(result_file),
                    '--configs-output', str(config_file)]
        result = subprocess.run([executable, *accepted, *common], env=environment,
                                capture_output=True, text=True, check=False)
        if result.returncode != 0:
            raise AssertionError(result.stderr)
        for path in (result_file, config_file):
            with path.open(newline='', encoding='utf-8') as stream:
                rows = list(csv.DictReader(stream))
            if len(rows) != 1 or rows[0]['preset'] != 'balanced_ttl35':
                raise AssertionError(('Label not preserved in CSV', path, rows))
        with config_file.open(newline='', encoding='utf-8') as stream:
            if next(csv.DictReader(stream))['pickup_lifetime'] != '35':
                raise AssertionError('Pickup lifetime override missing from configuration')

        invalid = [
            ['--preset', 'all', '--label', 'x'],
            ['--preset', 'balanced,repair', '--label', 'x'],
            ['--preset', 'balanced', '--label', ''],
            ['--preset', 'balanced', '--label', 'bad,label'],
            ['--preset', 'balanced', '--label', 'bad\nlabel'],
            ['--preset', 'balanced', '--profile', 'unknown'],
            ['--preset', 'unknown'],
            ['--preset', 'balanced,'],
            ['--preset', ',balanced'],
            ['--preset', 'balanced,,repair'],
            ['--preset', 'balanced', '--profile', 'beginner,'],
            ['--preset', 'balanced', '--pickup-lifetime', '0'],
        ]
        for arguments in invalid:
            rejected = subprocess.run([executable, *arguments, *common], env=environment,
                                      capture_output=True, text=True, check=False)
            if rejected.returncode == 0:
                raise AssertionError(('Invalid CLI accepted', arguments))

        original = result_file.read_bytes()
        # Distinct strings resolving to the same file must never truncate prior results.
        same_path = str(directory / 'missing' / '..' / 'results.csv')
        aliases = [(str(result_file), same_path)]
        symlink = directory / 'symlink.csv'
        symlink.symlink_to(result_file)
        aliases.append((str(result_file), str(symlink)))
        hardlink = directory / 'hardlink.csv'
        os.link(result_file, hardlink)
        aliases.append((str(result_file), str(hardlink)))
        for output, configs in aliases:
            rejected = subprocess.run([executable, '--preset', 'balanced', '--profile', 'beginner',
                                       '--output', output, '--configs-output', configs, *common],
                                      env=environment, capture_output=True, text=True, check=False)
            if rejected.returncode == 0 or result_file.read_bytes() != original:
                raise AssertionError(('Same CSV alias accepted or original data changed', output, configs))
    print('Simulator CLI: label, filters and CSV alias protection passed')


if __name__ == '__main__':
    main()
