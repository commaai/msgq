#!/usr/bin/env python3
import time
import random
import unittest

import cereal.messaging as messaging
from cereal.messaging.tests.test_messaging import events, random_sock, random_socks, random_bytes

class TestSubMaster(unittest.TestCase):

  def test_init(self):
    messaging.SubMaster(events)

  def test_init_state(self):
    sm = messaging.SubMaster(random_socks())
    self.assertEqual(sm.frame, -1)
    self.assertFalse(any(sm.updated.values()))
    self.assertFalse(any(sm.alive.values()))
    self.assertTrue(all(t == 0. for t in sm.rcv_time.values()))
    self.assertTrue(all(f == 0 for f in sm.rcv_frame.values()))
    self.assertTrue(all(t == 0 for t in sm.logMonoTime.values()))

  def test_getitem(self):
    pass

  def test_update(self):
    pass

  def test_update_timeout(self):
    pass

  def test_alive(self):
    pass

  def test_ignore_alive(self):
    pass

  def test_valid(self):
    pass

  # SubMaster should always conflate
  def test_conflate(self):
    sock = "carState"
    pub_sock = messaging.pub_sock(sock)
    sm = messaging.SubMaster([sock,])

    n = 10
    for i in range(n+1):
      msg = messaging.new_message(sock)
      msg.carState.vEgo = i
      pub_sock.send(msg.to_bytes())
      time.sleep(0.01)
    sm.update(2000)
    self.assertEqual(sm[sock].vEgo, n)


class TestPubMaster(unittest.TestCase):

  def test_init(self):
    messaging.PubMaster(events)

  def test_send(self):
    socks = random_socks()
    pm = messaging.PubMaster(socks)
    sub_socks = {s: messaging.sub_sock(s, timeout=1000) for s in socks}

    # PubMaster accepts either a capnp msg builder or bytes
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
