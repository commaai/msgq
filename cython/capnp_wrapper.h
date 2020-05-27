#pragma once

#include <cstring>
#include <capnp/serialize.h>
#include "../gen/cpp/car.capnp.h"

template<typename T>
T ReaderFromBytes(char * data, size_t sz){
  const size_t size = sz / sizeof(capnp::word) + 1;

  auto msg_reader = new capnp::FlatArrayMessageReader(kj::ArrayPtr<capnp::word>((capnp::word*)data, size));
  return msg_reader->getRoot<typename T::Reads>();
}


template<typename T>
using List = typename capnp::List<T>::Reader;

template<typename T>
using StructList = typename capnp::List<typename T::Reads>::Reader;
