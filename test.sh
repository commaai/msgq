#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null && pwd)"
cd $DIR

# *** env setup ***
source ./setup.sh

# *** build ***
scons -j8

# *** lint ***
ty check .
ruff check .

codespell -L ned,stdio --builtin clear,rare,informal,usage,code,names,en-GB_to_en-US \
          --skip uv.lock,*_pyx.cpp,catch2*

cppcheck --error-exitcode=1 --inline-suppr --language=c++ \
         --force --quiet -j4 --check-level=exhaustive \
         --suppress='*:msgq/catch2/*' --suppress='*:*_pyx.cpp' --suppress='*:*_tests.cc' msgq/ visionipc/

# *** test ***
msgq/test_runner
pytest
