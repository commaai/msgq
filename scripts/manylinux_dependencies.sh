#!/bin/bash

dnf install -y clang opencl-headers ocl-icd-devel cppcheck wget

wget https://github.com/zeromq/libzmq/releases/download/v4.3.5/zeromq-4.3.5.tar.gz
tar -xvf zeromq-4.3.5.tar.gz
cd zeromq-4.3.5
./configure
make -j$(nproc)
make install
