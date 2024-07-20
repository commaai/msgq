#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
ROOT=$DIR/../

brew bundle --file=- <<-EOS
brew "zeromq"
cask "gcc-arm-embedded"
brew "gcc@13"
EOS

if [[ -n "$NO_CATCH2" ]]; then
  cd /tmp
  git clone -b v2.x --depth 1 https://github.com/catchorg/Catch2.git
  cd Catch2
  mv single_include/* "$ROOT"
  rm -rf Catch2
fi
