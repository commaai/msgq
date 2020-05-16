#pragma once

#include "../gen/cpp/car.capnp.h"

cereal::CarState::Reader CarStateFromBytes(char * dat, size_t sz);
