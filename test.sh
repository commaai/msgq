#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null && pwd)"
cd $DIR

# *** env setup ***
source ./setup.sh

# *** build ***
scons -j8

# *** lint ***
ruff check .

codespell -L ned --builtin clear,rare,informal,usage,code,names,en-GB_to_en-US

cppcheck --error-exitcode=1 --inline-suppr --language=c++ \
         --force --quiet -j4 --check-level=exhaustive

# *** test ***
msgq/test_runner
pytest
