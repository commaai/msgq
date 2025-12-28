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
#ruff check .
pre-commit run --all-files

# *** test ***
msgq/test_runner
pytest
