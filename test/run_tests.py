#!/usr/bin/env python3

import sys
import argparse
import os
import re
import subprocess
import shlex
from glob import iglob
from shutil import get_terminal_size
from collections import (OrderedDict, Counter)
from concurrent.futures import ThreadPoolExecutor

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
        if not line.startswith('#'):
            yield None
        if line.startswith('# XFAIL:'):
            yield 'XFAIL'
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
    num_runs = 0
    output = []
    verbose_output = []
    banner = test_filename
    xfail = False
    with open(test_filename) as test_file:
        run_lines = []
        for line in parse_run_lines(test_file, test_filename,
                                    columbo_binary_path):
            if not line:
                break
            if line == 'XFAIL':
                xfail = True
            else:
                run_lines.append(line)
        try:
            for line in run_lines:
                cmd = ['bash', '-o', 'pipefail', '-c', line]
                verbose_output.append(f'STEP #{num_runs}: ' + shlex.join(cmd))
                num_runs += 1
                try:
                    subprocess.run(cmd, 
                                   check=True, capture_output=True)
                except subprocess.CalledProcessError as e:
                    output.append('\n'.join(verbose_output))
                    output.append(f'RETCODE: {e.returncode}')
                    if e.stdout:
                        output.append('PROCESS STDOUT:')
                        output.append(e.stdout.decode())
                    if e.stderr:
                        output.append('PROCESS STDERR:')
                        output.append(e.stderr.decode())
                    if xfail and e.returncode == 1:
                        print(banner + rjust + '[\033[34mXFAILED\033[0m]')
                        if SUPER_VERBOSE:
                            print('\n'.join(output), file=sys.stderr)
                        return 'xfailed', e.returncode, test_filename
                    print(banner + rjust + '[\033[31mFAILED\033[0m]')
                    print('\n'.join(output), file=sys.stderr)
                    return 'failed', e.returncode, test_filename
        except ColumboRunLineException as e:
            print(banner + rjust + '[\033[33mUNRESOLVED\033[0m]')
            if SUPER_VERBOSE:
                print(f'\tCould not parse run line: {e.message}', file=sys.stderr)
            return 'unresolved', 0, test_filename

    if num_runs == 0:
        print(banner + rjust + '[\033[33mSKIPPED\033[0m]')
        return 'skipped', 0, test_filename
    if xfail:
        print(banner + rjust + '[\033[36mXPASSED\033[0m]')
    else:
        print(banner + rjust + '[\033[32mPASSED\033[0m]')
    if SUPER_VERBOSE:
        print('\n'.join(verbose_output), file=sys.stderr)
    return 'xpassed' if xfail else 'passed', 0, test_filename


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
    VERBOSE = args.verbose or args.very_verbose

    global SUPER_VERBOSE
    SUPER_VERBOSE = args.very_verbose

    if os.path.basename(args.columbo_binary) != 'columbo':
        print('Unexpected columbo binary name: '
              f'{os.path.basename(args.columbo_binary)}')
        return 1

    if not os.path.exists(args.columbo_binary):
        print(f'Cannot find columbo binary: {args.columbo_binary}')
        return 1

    with ThreadPoolExecutor(max_workers = 8) as executor:
        args = [(t, os.path.abspath(args.columbo_binary)) for t in tests]
        results = executor.map(lambda p: run_test(*p), args)

    l = list(results)
    c = Counter([k for k,v,t in l])

    print('=== SUMMARY ===')
    if c.get("unresolved", 0) > 0:
        print('  UNRESOLVED TESTS:')
        print('\t' + "\n\t".join(t for k,v,t in filter(lambda v: v[0] == 'unresolved', l)))
    if c.get("skipped", 0) > 0:
        print('  SKIPPED TESTS:')
        print('\t' + "\n\t".join(t for k,v,t in filter(lambda v: v[0] == 'skipped', l)))
    if c.get("xpassed", 0) > 0:
        print('  XPASSED TESTS:')
        print('\t' + "\n\t".join(t for k,v,t in filter(lambda v: v[0] == 'xpassed', l)))
    if c.get("failed", 0) > 0:
        print('  FAILED TESTS:')
        print('\t' + "\n\t".join(t for k,v,t in filter(lambda v: v[0] == 'failed', l)))
    print(f'  PASSED: {c.get("passed", 0)}')
    print(f'  FAILED: {c.get("failed", 0)}')
    print(f'  XPASSED: {c.get("xpassed", 0)}')
    print(f'  XFAILED: {c.get("xfailed", 0)}')
    print(f'  SKIPPED: {c.get("skipped", 0)}')
    print(f'  UNRESOLVED: {c.get("unresolved", 0)}')

    return 1 if c.get("failed", 0) > 0 else 0


if __name__ == '__main__':
    sys.exit(main())
