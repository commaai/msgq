#!/usr/bin/env python3
import os
import unittest
from parameterized import parameterized

import cereal.services as services
from cereal.services import service_list


class TestServices(unittest.TestCase):

  @parameterized.expand(service_list.keys())
  def test_services(self, s):
    service = service_list[s]
    self.assertTrue(service.port > 8000)
    self.assertTrue(service.frequency <= 100)

  def test_no_duplicate_port(self):
    ports = {}
    for name, service in service_list.items():
      self.assertFalse(service.port in ports.keys(), f"duplicate port {service.port}")
      ports[service.port] = name

  # TODO: check that the header is valid C
  def test_generated_header(self):
    ret = os.system(f"python3 {services.__file__} > /dev/null")
    self.assertEqual(ret, 0)

if __name__ == "__main__":
  unittest.main()
