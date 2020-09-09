#!/bin/bash
set -e
echo "Installing capnp"

cd /tmp
rm -rf capnp*
VERSION=0.6.1
wget https://capnproto.org/capnproto-c++-${VERSION}.tar.gz
tar xvf capnproto-c++-${VERSION}.tar.gz
cd capnproto-c++-${VERSION}
CXXFLAGS="-fPIC" ./configure
make -j$(nproc)
sudo make install

if [ $1 == "arm64" ]; then
    #Now clean and build for aarch64

    echo -e "\e[1;33m Building CAPNP for ARM64... \e[0m"

    make clean 
    make distclean 

    CXXFLAGS="-fPIC" ./configure --with-external-capnp --host=aarch64-linux-gnu --prefix=/usr/aarch64-linux-gnu
    make -j$(nproc)
    sudo make install


    # manually build binaries statically
    aarch64-linux-gnu-g++ -std=gnu++11 -I./src -I./src -DKJ_HEADER_WARNINGS -DCAPNP_HEADER_WARNINGS -DCAPNP_INCLUDE_DIR=\"/usr/local/include\" -pthread -O2 -DNDEBUG -o .libs/capnp src/capnp/compiler/module-loader.o src/capnp/compiler/capnp.o  ./.libs/libcapnpc.a ./.libs/libcapnp.a ./.libs/libkj.a -lpthread -pthread
    aarch64-linux-gnu-g++ -std=gnu++11 -I./src -I./src -DKJ_HEADER_WARNINGS -DCAPNP_HEADER_WARNINGS -DCAPNP_INCLUDE_DIR=\"/usr/local/include\" -pthread -O2 -DNDEBUG -o .libs/capnpc-c++ src/capnp/compiler/capnpc-c++.o  ./.libs/libcapnp.a ./.libs/libkj.a -lpthread -pthread
    aarch64-linux-gnu-g++ -std=gnu++11 -I./src -I./src -DKJ_HEADER_WARNINGS -DCAPNP_HEADER_WARNINGS -DCAPNP_INCLUDE_DIR=\"/usr/local/include\" -pthread -O2 -DNDEBUG -o .libs/capnpc-capnp src/capnp/compiler/capnpc-capnp.o  ./.libs/libcapnp.a ./.libs/libkj.a -lpthread -pthread

    cp .libs/capnp /usr/aarch64-linux-gnu/bin/
    ln -snf /usr/aarch64-linux-gnu/bin/capnp /usr/aarch64-linux-gnu/bin/capnpc
    cp .libs/capnpc-c++ /usr/aarch64-linux-gnu/bin/
    cp .libs/capnpc-capnp /usr/aarch64-linux-gnu/bin/
    cp .libs/*.a /usr/aarch64-linux-gnu/lib

    cd /tmp
    echo "Installing c-capnp"
    git clone https://github.com/commaai/c-capnproto.git
    cd c-capnproto
    git submodule update --init --recursive
    autoreconf -f -i -s
    CXXFLAGS="-fPIC" ./configure --host=aarch64-linux-gnu --with-external-capnp --prefix=/usr/aarch64-linux-gnu
    make -j$(nproc)
    sudo make install

    # manually build binaries statically
    aarch64-linux-gnu-gcc -fPIC -o .libs/capnpc-c compiler/capnpc-c.o compiler/schema.capnp.o compiler/str.o  ./.libs/libcapnp_c.a

    cp -nf .libs/capnpc-c /usr/aarch64-linux-gnu/bin/
    cp -nf .libs/*.a /usr/aarch64-linux-gnu/lib

    echo -e "\e[1;32m CAPNP for ARM64 built and installed... \e[0m"
fi

echo -e "\e[1;32m CAPNP setup completed! \e[0m"

cd /tmp
rm -rf capnp*
