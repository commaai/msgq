#!/bin/bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null && pwd)"
cd $DIR

source ./setup.sh

# *** build ***
scons -j8

# Verify the package is installable and is not mislabeled as platform-independent.
rm -rf dist
uv build --wheel
wheel="$(find dist -maxdepth 1 -name '*.whl' -print -quit)"
if [[ "$wheel" == *-none-any.whl ]]; then
  echo "native extension wheel is incorrectly tagged as platform-independent: $wheel" >&2
  exit 1
fi
uv run --isolated --no-project --with "$wheel" --directory / python -c \
  "import msgq, msgq.visionipc"

# *** lint + test ***
lefthook run test

# *** all done ***
GREEN='\033[0;32m'
NC='\033[0m'
printf "\n${GREEN}All good!${NC} Finished build, lint, and test in ${SECONDS}s\n"
