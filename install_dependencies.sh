#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"

cd $DIR

SUDO=""

# Use sudo if not root
if [[ ! $(id -u) -eq 0 ]]; then
  if [[ -z $(which sudo) ]]; then
    echo "Please install sudo or run as root"
    exit 1
  fi
  SUDO="sudo"
fi

$SUDO apt-get update
$SUDO apt-get install -y --no-install-recommends \
  autoconf \
  build-essential \
  ca-certificates \
  capnproto \
  clang \
  cppcheck \
  curl \
  git \
  libbz2-dev \
  libcapnp-dev \
  libclang-rt-dev \
  libffi-dev \
  liblzma-dev \
  libncurses5-dev \
  libncursesw5-dev \
  libreadline-dev \
  libsqlite3-dev \
  libssl-dev \
  libtool \
  libzmq3-dev \
  llvm \
  make \
  cmake \
  ocl-icd-opencl-dev \
  opencl-headers  \
  python3-dev \
  python3-pip \
  tk-dev \
  wget \
  xz-utils \
  zlib1g-dev

git clone -b v2.x --depth 1 https://github.com/catchorg/Catch2.git
cd Catch2
mv single_include/* ../
cd ..
rm -rf Catch2
