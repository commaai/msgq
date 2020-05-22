#!/usr/bin/env python3

import time

import cython.log as cython_log
from cereal import car


cs = car.CarState.new_message()
cs.brake = 10

t = time.time()
for _ in range(100000):
  a = cs.brake
print("pycapnp", time.time() - t)


b = cs.to_bytes()
cs = cython_log.CarState(b)

t = time.time()
for _ in range(100000):
  a = cs.brake
print("cython", time.time() - t)


# Nested
cs = car.CarState.new_message()
cs.cruiseState.speed = 1234
t = time.time()
for _ in range(100000):
  a = cs.cruiseState.speed
print("nested pycapnp", time.time() - t)


b = cs.to_bytes()
cs = cython_log.CarState(b)

t = time.time()
for _ in range(100000):
  a = cs.cruiseState.speed
print("nested cython", time.time() - t)
