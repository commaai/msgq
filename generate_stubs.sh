#!/bin/bash

INSTALLED=$(pip list | grep capnp-stub-generator)

if [ "$1" == "--dev" ] || [ -z "$INSTALLED" ]; then
  FORCE_INSTALL=1
else
  FORCE_INSTALL=0
fi

set -e
if [ "$FORCE_INSTALL" -eq 1 ]; then
  [[ -z "$INSTALLED" ]] || pip uninstall -y capnp-stub-generator
  pip install git+https://github.com/nworb-cire/capnp-stub-generator.git@openpilot-tweaks
fi
capnp-stub-generator -p "*.capnp" -c "(car|log|maptile|legacy|custom).py" "*.pyi"

for file in "car" "log" "maptile" "legacy" "custom"
do
  mv ${file}_capnp.pyi ${file}.pyi
  mv ${file}_capnp.py ${file}.py
  # Prepend autogenerated files with noqa statement to ignore ruff errors
  echo "# ruff: noqa: A003, F821, E501" | cat - ${file}.pyi > temp && mv temp ${file}.pyi
done
