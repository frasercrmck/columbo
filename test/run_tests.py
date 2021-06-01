#!/usr/bin/env python3

import sys
import argparse
import os
import re
import subprocess
import shlex
from glob import iglob
from shutil import get_terminal_size
from collections import OrderedDict

TEST_ROOT = os.path.dirname(os.path.abspath(__file__))

DEFAULT_TEST_PATHS = [
    os.path.join(TEST_ROOT, 'test_sudokus')
]

VERBOSE = False
SUPER_VERBOSE = False
COLS = get_terminal_size().columns


class ColumboRunLineException(Exception):
    def __init__(self, message):
        self.message = message
        super().__init__(self.message)
    pass


def parse_run_lines(test_file, test_filename, columbo_binary_path):
    for line in test_file.readlines():
        if not line.startswith('# RUN:'):
            yield None
        line = line[6:].lstrip()
        for comp in map(str.lstrip, line.split('|')):
            if not re.match(r'columbo(_check)?\b', comp):
                raise ColumboRunLineException(f"Subcomponent '{comp}' does not "
                                               "run either 'columbo' or 'columbo_check'")
        line = re.sub(r'\bcolumbo\b', columbo_binary_path, line)
        line = re.sub(r'\bcolumbo_check\b', f'{os.path.join(TEST_ROOT, "columbo_check.py")}', line)
        line = re.sub(r'%S', os.path.join(TEST_ROOT, os.path.dirname(test_filename)), line)
        line = re.sub(r'%s', os.path.join(TEST_ROOT, test_filename), line)
        line = re.sub(r'%%', '%', line)
        m = re.search(r'%\w', line)
        if m:
            raise ColumboRunLineException(f"Unrecognized substitution '{m.group(0)}'")
        line = line.rstrip()
        yield line


def run_test(test_filename, columbo_binary_path):
    l = len(test_filename)
    rjust = (COLS - l - 12) * ' '
    print(test_filename, end='')
    num_runs = 0
    super_verbose_output = []
    with open(test_filename) as test_file:
        try:
            for line in parse_run_lines(test_file, test_filename,
                                        columbo_binary_path):
                if not line:
                    break
                cmd = ['bash', '-o', 'pipefail', '-c', line]
                if SUPER_VERBOSE:
                    super_verbose_output.append(f'STEP #{num_runs}: ' + shlex.join(cmd))
                num_runs += 1
                try:
                    subprocess.run(cmd, 
                                   check=True, capture_output=True)
                except subprocess.CalledProcessError as e:
                    print(rjust + '[\033[31mFAILED\033[0m]')
                    if SUPER_VERBOSE:
                        print('\n'.join(super_verbose_output), file=sys.stderr)
                    if VERBOSE and e.stdout:
                        print('PROCESS STDOUT:')
                        print(e.stdout.decode(), file=sys.stdout)
                    if VERBOSE and e.stderr:
                        print('PROCESS STDERR:')
                        print(e.stderr.decode(), file=sys.stderr)
                    return 'failed', e.returncode
        except ColumboRunLineException as e:
            print(rjust + '[\033[33mUNRESOLVED\033[0m]')
            if SUPER_VERBOSE:
                print(f'\tCould not parse run line: {e.message}')
            return 'unresolved', 0

    if num_runs == 0:
        print(rjust + '[\033[33mSKIPPED\033[0m]')
        return 'skipped', 0
    print(rjust + '[\033[32mPASSED\033[0m]')
    if SUPER_VERBOSE:
        print('\n'.join(super_verbose_output), file=sys.stdout)
    return 'passed', 0


def ReadableDirOrFile(prospective_path):
    if not os.path.isdir(prospective_path) and not os.path.isfile(prospective_path):
        raise argparse.ArgumentTypeError(f"'{prospective_path}' is not a valid path")
    if os.access(prospective_path, os.R_OK):
        return prospective_path
    raise argparse.ArgumentTypeError(f"'{prospective_path}' is not a readable file or directory")


def main():
    parser = argparse.ArgumentParser(description='Run tests')
    parser.add_argument('--verbose', '-v', action='store_true', default=False)
    parser.add_argument('--very-verbose', '-vv', action='store_true', default=False)
    parser.add_argument('--columbo-binary', required=True)
    parser.add_argument('test_paths', type=ReadableDirOrFile, nargs='*')

    args = parser.parse_args()

    tests = OrderedDict()
    for test_path in args.test_paths or DEFAULT_TEST_PATHS:
        if os.path.isfile(test_path):
            tests[os.path.abspath(test_path)] = None
        else:
            for test_file_path in iglob(os.path.join(test_path, '*.txt'),
                                        recursive=False):
                tests[os.path.abspath(test_file_path)] = None

    global VERBOSE
    VERBOSE = args.verbose

    global SUPER_VERBOSE
    SUPER_VERBOSE = args.very_verbose

    if os.path.basename(args.columbo_binary) != 'columbo':
        print('Unexpected columbo binary name: '
              f'{os.path.basename(args.columbo_binary)}')
        return 1

    if not os.path.exists(args.columbo_binary):
        print(f'Cannot find columbo binary: {args.columbo_binary}')
        return 1

    stats = {
        'passed' : 0,
        'failed' : 0,
        'skipped' : 0,
        'unresolved' : 0,
    }
    for test_file_path in tests:
        kind, errcode = run_test(test_file_path, os.path.abspath(args.columbo_binary))
        stats[kind ] = stats.get(kind, 0) + 1

    print('=== SUMMARY ===')
    print(f'  PASSED: {stats["passed"]}')
    print(f'  FAILED: {stats["failed"]}')
    print(f'  SKIPPED: {stats["skipped"]}')
    print(f'  UNRESOLVED: {stats["unresolved"]}')

    return 1 if stats["failed"] > 0 else 0


if __name__ == '__main__':
    sys.exit(main())
