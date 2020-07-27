#!/usr/bin/env python3
import os
import time
import random
import unittest
import numbers
import capnp
from parameterized import parameterized

from cereal import log, car
import cereal.messaging as messaging
from cereal.services import service_list

events = [evt for evt in log.Event.schema.union_fields if evt in service_list.keys()]

def random_sock():
  return random.choice(events)

def random_socks(num_socks=10):
  return list(set([random_sock() for _ in range(num_socks)]))

def random_bytes(length=1000):
  return bytes([random.randrange(0xFF) for _ in range(length)])

def zmq_sleep(t=1):
  if "ZMQ" in os.environ:
    time.sleep(t)

# TODO: this should take any capnp struct and returrn a msg with random populated data
def random_carstate():
  fields = ["vEgo", "aEgo", "gas", "steeringAngle"]
  msg = messaging.new_message("carState")
  cs = msg.carState
  for f in fields:
    setattr(cs, f, random.random() * 10)
  return msg

# TODO: this should compare any capnp structs
def assert_carstate(cs1, cs2):
  for f in car.CarState.schema.non_union_fields:
    # TODO: check all types
    val1, val2 = getattr(cs1, f), getattr(cs2, f)
    if isinstance(val1, numbers.Number):
      assert val1 == val2, f"{f}: sent '{val1}' vs recvd '{val2}'"


class TestPubSubSockets(unittest.TestCase):

  def setUp(self):
    # ZMQ pub socket takes too long to die
    # sleep to prevent multiple publishers error between tests
    zmq_sleep()

  def test_pub_sub(self):
    sock = random_sock()
    pub_sock = messaging.pub_sock(sock)
    sub_sock = messaging.sub_sock(sock, conflate=False, timeout=None)
    zmq_sleep(3)

    for _ in range(1000):
      msg = random_bytes()
      pub_sock.send(msg)
      recvd = sub_sock.receive()
      self.assertEqual(msg, recvd)

  def test_conflate(self):
    sock = random_sock()
    pub_sock = messaging.pub_sock(sock)
    for conflate in [True, False]:
      for _ in range(10):
        num_msgs = random.randint(3, 10)
        sub_sock = messaging.sub_sock(sock, conflate=conflate, timeout=None)
        zmq_sleep()

        sent_msgs = []
        for __ in range(num_msgs):
          msg = random_bytes()
          pub_sock.send(msg)
          sent_msgs.append(msg)
        time.sleep(0.1)
        recvd_msgs = messaging.drain_sock_raw(sub_sock)
        if conflate:
          self.assertEqual(len(recvd_msgs), 1)
        else:
          # TODO: compare actual data
          self.assertEqual(len(recvd_msgs), len(sent_msgs))

  def test_receive_timeout(self):
    sock = random_sock()
    for _ in range(10):
      timeout = random.randrange(200)
      sub_sock = messaging.sub_sock(sock, timeout=timeout)
      zmq_sleep()

      start_time = time.monotonic()
      recvd = sub_sock.receive()
      self.assertLess(time.monotonic() - start_time, 0.2)
      assert recvd is None

class TestMessaging(unittest.TestCase):

  def setUp(self):
    # ZMQ pub socket takes too long to die
    # sleep to prevent multiple publishers error between tests
    zmq_sleep()

  @parameterized.expand(events)
  def test_new_message(self, evt):
    try:
      msg = messaging.new_message(evt)
    except capnp.lib.capnp.KjException:
      msg = messaging.new_message(evt, random.randrange(200))
    self.assertLess(time.monotonic() - msg.logMonoTime, 0.1)
    self.assertTrue(msg.valid)
    self.assertEqual(evt, msg.which())

  @parameterized.expand(events)
  def test_pub_sock(self, evt):
    messaging.pub_sock(evt)

  @parameterized.expand(events)
  def test_sub_sock(self, evt):
    messaging.sub_sock(evt)

  @parameterized.expand([
    (messaging.drain_sock, capnp._DynamicStructReader),
    (messaging.drain_sock_raw, bytes),
  ])
  def test_drain_sock(self, func, expected_type):
    sock = "carState"
    pub_sock = messaging.pub_sock(sock)
    sub_sock = messaging.sub_sock(sock, timeout=1000)
    zmq_sleep()

    # TODO: test wait_for_one

    # no wait and no msgs in queue
    msgs = func(sub_sock)
    self.assertTrue(isinstance(msgs, list))
    self.assertEqual(len(msgs), 0)

    # no wait but msgs are queued up
    num_msgs = random.randrange(3, 10)
    for _ in range(num_msgs):
      pub_sock.send(messaging.new_message(sock).to_bytes())
    time.sleep(0.1)
    msgs = func(sub_sock)
    self.assertTrue(isinstance(msgs, list))
    self.assertTrue(all(isinstance(msg, expected_type) for msg in msgs))
    self.assertEqual(len(msgs), num_msgs)

  def test_recv_sock(self):
    sock = "carState"
    pub_sock = messaging.pub_sock(sock)
    sub_sock = messaging.sub_sock(sock, timeout=1000)
    zmq_sleep()

    # TODO: test wait

    # no wait, socket should timeout
    recvd = messaging.recv_sock(sub_sock)
    self.assertTrue(recvd is None)

    # no wait, one msg in queue 
    msg = random_carstate()
    pub_sock.send(msg.to_bytes())
    recvd = messaging.recv_sock(sub_sock)
    self.assertTrue(isinstance(recvd, capnp._DynamicStructReader))
    assert_carstate(msg.carState, recvd.carState)

  def test_recv_one(self):
    pass

  def test_recv_one_or_none(self):
    pass

  def test_recv_one_retry(self):
    pass

if __name__ == "__main__":
  unittest.main()
