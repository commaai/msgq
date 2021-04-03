set -e
echo "Installing capnp"

cd /tmp
VERSION=0.8.0
wget https://capnproto.org/capnproto-c++-${VERSION}.tar.gz
tar xvf capnproto-c++-${VERSION}.tar.gz
cd capnproto-c++-${VERSION}
CXXFLAGS="-fPIC" ./configure

make -j$(nproc)
make install
