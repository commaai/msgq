#pragma once

#include <cstring>
#include <capnp/serialize.h>
#include "../gen/cpp/car.capnp.h"

template<typename T>
T ReaderFromBytes(char * dat, size_t sz){
  const size_t size = sz / sizeof(capnp::word) + 1;

  // TODO: what happens when buf goes out of scope? Is the memory retained by the reader?
  auto buf = kj::heapArray<capnp::word>(size);
  memcpy(buf.begin(), dat, sz);

  auto msg_reader = new capnp::FlatArrayMessageReader(kj::ArrayPtr<capnp::word>(buf.begin(), size));
  return msg_reader->getRoot<typename T::Reads>();
}
