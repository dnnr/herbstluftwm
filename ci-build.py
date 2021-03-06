#!/usr/bin/env python3

import argparse
import os
import re
import subprocess as sp
import sys
from pathlib import Path


def tox(tox_args, build_dir):
    """
    Prepare environment for tox and execute it with the given arguments
    """
    tox_env = os.environ.copy()
    tox_env['PWD'] = build_dir
    sp.check_call(f'tox {tox_args}', shell=True, cwd=build_dir, env=tox_env)


parser = argparse.ArgumentParser()
parser.add_argument('--build-dir', type=str, required=True)
parser.add_argument('--build-type', type=str, choices=('Release', 'Debug'), required=True)
parser.add_argument('--compile', action='store_true')
parser.add_argument('--run-tests', action='store_true')
parser.add_argument('--build-docs', action='store_true')
parser.add_argument('--cxx', type=str)
parser.add_argument('--cc', type=str)
parser.add_argument('--check-using-std', action='store_true')
parser.add_argument('--iwyu', action='store_true')
parser.add_argument('--flake8', action='store_true')
parser.add_argument('--ccache', nargs='?', metavar='ccache dir', type=str,
                    const=os.environ.get('CCACHE_DIR') or True)
args = parser.parse_args()

build_dir = Path(args.build_dir)
build_dir.mkdir(exist_ok=True)

if args.check_using_std:
    sp.check_call(['./ci-check-using-std.sh'], cwd='/hlwm')

if args.ccache:
    if args.ccache is not True:
        os.environ['CCACHE_DIR'] = args.ccache

    # Delete config to prevent carrying over state unintentionally
    conf = Path(os.environ.get('CCACHE_DIR') or (Path.home() / '.ccache')) / 'ccache.conf'
    if conf.exists():
        conf.unlink()

    # Set a reasonable size limit
    sp.check_call('ccache --max-size=500M', shell=True)

    # Wipe stats before build
    sp.check_call('ccache -z', shell=True)

    # Print config for confirmation:
    sp.check_call('ccache -p', shell=True)

build_env = os.environ.copy()
build_env.update({
    'CC': args.cc,
    'CXX': args.cxx,
})

cmake_args = [
    '-GNinja',
    '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
    f'-DCMAKE_BUILD_TYPE={args.build_type}',
    f'-DWITH_DOCUMENTATION={"YES" if args.build_docs else "NO"}',
    f'-DENABLE_CCACHE={"YES" if args.ccache else "NO"}',
]

sp.check_call(['cmake', *cmake_args, '..'], cwd=build_dir, env=build_env)

if args.compile:
    sp.check_call(['bash', '-c', 'time ninja -v -k 10'], cwd=build_dir, env=build_env)

if args.ccache:
    sp.check_call(['ccache', '-s'])

if args.iwyu:
    # Check lexicographical order of #include directives (cheap pre-check)
    fix_include = sp.run('fix_include --dry_run --sort_only --reorder /hlwm/*/*.{h,cpp,c}', shell=True, executable='bash', stdout=sp.PIPE)
    print(re.sub(
        r">>> Fixing #includes in '[^']*'[\n\r]*No changes in file [^\n\r]*[\n\r]*",
        '',
        fix_include.stdout.decode()))
    if fix_include.returncode != 0:
        print('IWYU/fix_include made suggestions, please fix them')
        sys.exit(1)

    # Run include-what-you-use
    iwyu_out = sp.check_output(f'iwyu_tool -p . -j "$(nproc)" -- --transitive_includes_only --mapping_file=/hlwm/.hlwm.imp', shell=True, cwd=build_dir)

    # Check IWYU output, but ignore any suggestions to add things (those tend
    # to be overzealous):
    complaints = set(re.findall(r'(\S+) should remove these lines:\n[^\n]', iwyu_out.decode('ascii')))
    if complaints:
        sys.stdout.buffer.write(iwyu_out)
        print('IWYU made suggestions to remove things in (see log above): {}'.format(', '.join(complaints)))
        sys.exit(1)

if args.flake8:
    tox('-e flake8', build_dir)

if args.run_tests:
    tox('-e py37 -- -n auto --cache-clear -v -x', build_dir)

    sp.check_call('lcov --capture --directory . --output-file coverage.info', shell=True, cwd=build_dir)
    sp.check_call('lcov --remove coverage.info "/usr/*" --output-file coverage.info', shell=True, cwd=build_dir)
    sp.check_call('lcov --list coverage.info', shell=True, cwd=build_dir)
    (build_dir / 'coverage.info').rename('/hlwm/coverage.info')
