#!/usr/bin/env python3

import time

import cython.car as cython_car
from cereal import car


cs = car.CarState.new_message()
cs.brake = 10

t = time.time()
for _ in range(100000):
  a = cs.brake
print(time.time() - t)


b = cs.to_bytes()
cs = cython_car.CarState(b)

t = time.time()
for _ in range(100000):
  a = cs.brake
print(time.time() - t)
