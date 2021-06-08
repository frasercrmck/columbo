#!/usr/bin/env python3

import sys
import argparse
from difflib import unified_diff

def is_ignored_line(line):
    return not line.lstrip().startswith('#')


def strip(line):
    return line.strip(" \t")


def main():
    parser = argparse.ArgumentParser(description='Check tests')

    parser.add_argument('-input-file', nargs='?',
                        type=argparse.FileType('r'), default=sys.stdin)
    parser.add_argument('check_file', type=argparse.FileType('r'))

    args = parser.parse_args()

    errcode = 0
    check_lines = list(map(strip, filter(is_ignored_line, args.check_file.readlines())))
    input_lines = list(map(strip, filter(is_ignored_line, args.input_file.readlines())))
    for line in unified_diff(check_lines, input_lines,
                             fromfile='check_file', tofile='input'):
        errcode = 1
        print(line, end='', file=sys.stderr)

    return errcode


if __name__ == '__main__':
    sys.exit(main())
