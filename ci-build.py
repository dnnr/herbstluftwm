#!/usr/bin/env python3

import argparse
import os
import shlex
import subprocess as sp

parser = argparse.ArgumentParser()
parser.add_argument('--build-type', type=str, choices=('Release', 'Debug'), required=True)
parser.add_argument('--run-tests', action='store_true')
parser.add_argument('--build-docs', action='store_true')
parser.add_argument('--check-using-std', action='store_true')
parser.add_argument('--ccache', action='store_true')
parser.add_argument('--cxx', type=str, required=True)
parser.add_argument('--cc', type=str, required=True)
args = parser.parse_args()

if args.check_using_std:
    sp.check_call(['./ci-check-using-std.sh'], cwd='/hlwm')

build_dir = '/hlwm/build'
os.makedirs(build_dir, exist_ok=True)

cmake_env = os.environ.copy()
cmake_env.update({
    'CC': args.cc,
    'CXX': args.cxx,
    })

cmake_args = [
    '-GNinja',
    f'-DCMAKE_BUILD_TYPE={args.build_type}',
    f'-DWITH_DOCUMENTATION={"YES" if args.build_docs else "NO"}',
    f'-DENABLE_CCACHE={"YES" if args.ccache else "NO"}',
    ]

sp.check_call(['cmake', *cmake_args, '..'], cwd=build_dir, env=cmake_env)

sp.check_call(shlex.split('ninja -v -k 10'), cwd=build_dir, env=cmake_env)

if args.run_tests:
    tox_env = os.environ.copy()
    tox_env.update({
        'PWD': build_dir,
        })
    sp.check_call(shlex.split(f'tox -e py37 -- -n 1 -v -x'), cwd=build_dir, env=tox_env)
