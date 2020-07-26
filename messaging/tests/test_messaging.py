#!/usr/bin/env python3
import time
import random
import unittest

import cereal.messaging as messaging
from cereal.services import service_list

def random_sock():
  return random.choice(list(service_list.keys()))

def random_bytes(length=1000):
  return bytes([random.randrange(0xFF) for __ in range(length)])

# TODO: test both msgq and zmq

class TestPubSubSockets(unittest.TestCase):

  def test_pub_sub(self):
    sock = random_sock()
    pub_sock = messaging.pub_sock(sock)
    sub_sock = messaging.sub_sock(sock, conflate=False, timeout=None)

    for _ in range(1000):
      msg = random_bytes()
      pub_sock.send(msg)
      recvd = sub_sock.receive()
      self.assertEqual(msg, recvd)

  def test_conflate(self):
    for conflate in [True, False]:
      for _ in range(10):
        sock = random_sock()
        num_msgs = random.randint(3, 10)
        pub_sock = messaging.pub_sock(sock)
        sub_sock = messaging.sub_sock(sock, conflate=conflate, timeout=None)

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

  def test_timeout(self):
    for _ in range(10):
      sock = random_sock()
      timeout = random.randrange(200)
      pub_sock = messaging.pub_sock(sock)
      sub_sock = messaging.sub_sock(sock, timeout=timeout)

      start_time = time.monotonic()
      recvd = sub_sock.receive()
      self.assertLess(time.monotonic() - start_time, 0.2)
      assert recvd is None

class TestSubMaster(unittest.TestCase):
  pass

class TestPubMaster(unittest.TestCase):

  def test_send(self):
    socks = list(set([random_sock() for _ in range(random.randrange(10))]))
    pm = messaging.PubMaster(socks)
    sub_socks = {s: messaging.sub_sock(s, timeout=1000) for s in socks}

    for capnp in [True, False]:
      for i in range(100):
        sock = socks[i % len(socks)]

        if capnp:
          try:
            msg = messaging.new_message(sock)
          except:
            msg = messaging.new_message(sock, random.randrange(50))
        else:
          msg = random_bytes()

        pm.send(sock, msg)
        recvd = sub_socks[sock].receive()

        if capnp:
          msg = msg.to_bytes()
        self.assertEqual(msg, recvd)


if __name__ == "__main__":
  unittest.main()
