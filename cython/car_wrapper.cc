#include <cstring>

#include <capnp/serialize.h>

#include "../gen/cpp/car.capnp.h"

// TODO: Template
cereal::CarState::Reader CarStateFromBytes(char * dat, size_t sz){
  const size_t size = sz / sizeof(capnp::word) + 1;
  auto buf = kj::heapArray<capnp::word>(size);

  memcpy(buf.begin(), dat, sz);

  auto msg_reader = new capnp::FlatArrayMessageReader(kj::ArrayPtr<capnp::word>(buf.begin(), size));
  return msg_reader->getRoot<cereal::CarState>();
}
