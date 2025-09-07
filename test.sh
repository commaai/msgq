#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null && pwd)"
cd $DIR

# *** env setup ***
source ./setup.sh

rm -rf /tmp/catch2/ $DIR/msgq/catch2/
git clone -b v2.x --depth 1 https://github.com/catchorg/Catch2.git /tmp/catch2
pushd /tmp/catch2
mv single_include/* $DIR/msgq/
popd

# *** build ***
scons -j8

# *** lint ***
#ruff check .
#mypy python/
pre-commit run --all-files

# *** test ***

# TODO: make randomly work
pytest --randomly-dont-reorganize tests/
