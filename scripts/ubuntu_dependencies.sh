#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
ROOT=$DIR/../

if [[ ! $(id -u) -eq 0 ]]; then
  if [[ -z $(which sudo) ]]; then
    echo "Please install sudo or run as root"
    exit 1
  fi
  SUDO="sudo"
fi

$SUDO apt-get update
$SUDO apt-get install -y --no-install-recommends clang opencl-headers libzmq3-dev ocl-icd-opencl-dev cppcheck

if [[ -n "$NO_CATCH2" ]]; then
  cd /tmp
  git clone -b v2.x --depth 1 https://github.com/catchorg/Catch2.git
  cd Catch2
  mv single_include/* "$ROOT"
  rm -rf Catch2
fi
