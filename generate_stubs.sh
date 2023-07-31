#!/bin/bash -e

pip install ./capnp-stub-generator
capnp-stub-generator -p "*.capnp" -c "*.pyi"

for file in "car" "log" "maptile" "legacy" "custom"
do
  mv ${file}_capnp.pyi ${file}.pyi
  mv ${file}_capnp.py ${file}.py
done
