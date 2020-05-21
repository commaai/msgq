import unittest

import cereal.cython.car as cython_car
from cereal import car


class TestStruct(unittest.TestCase):
  def test_struct(self):
    cs = car.CarState.new_message()
    cs.brake = 10.1
    cs.gasPressed = True

    b = cs.to_bytes()
    cs_cython = cython_car.CarState(b)

    self.assertAlmostEqual(cs.brake, cs_cython.brake)
    self.assertTrue(cs_cython.gasPressed)

  def test_nested_struct(self):
    cs = car.CarState.new_message()
    cs.cruiseState.enabled = True
    cs.cruiseState.speed = 1234.5

    b = cs.to_bytes()
    cs_cython = cython_car.CarState(b)

    self.assertAlmostEqual(cs.cruiseState.speed, cs_cython.cruiseState.speed)
    self.assertTrue(cs_cython.cruiseState.enabled)
