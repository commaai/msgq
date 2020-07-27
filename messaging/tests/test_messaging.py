#!/usr/bin/env python3
import os
import time
import random
import unittest
import capnp
from parameterized import parameterized

from cereal import log
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

# TODO: test both msgq and zmq

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
    self.assertEqual(evt, msg.which())

  @parameterized.expand(events)
  def test_pub_sock(self, evt):
    messaging.pub_sock(evt)

  @parameterized.expand(events)
  def test_sub_sock(self, evt):
    messaging.sub_sock(evt)

  def test_drain_sock_raw(self):
    sock = random_sock()
    pub_sock = messaging.pub_sock(sock)

  def test_drain_sock(self):
    pass

  def test_recv_sock(self):
    pass

  def test_recv_one(self):
    pass

  def test_recv_one_or_none(self):
    pass

  def test_recv_one_retry(self):
    pass

if __name__ == "__main__":
  unittest.main()
