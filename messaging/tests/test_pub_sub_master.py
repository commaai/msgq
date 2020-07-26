#!/usr/bin/env python3
import random
import unittest

import cereal.messaging as messaging
from cereal.services import service_list

class TestPubMaster(unittest.TestCase):

  def test_send(self):
    sock = random.choice(list(service_list.keys()))
    pm = messaging.PubMaster((sock, ))
    sub_sock = messaging.sub_sock(sock, timeout=1000)

    msg = messaging.new_message('carState')
    msg_bytes = msg.to_bytes()
    pm.send(sock, msg)
    recvd = messaging.drain_sock_raw(sub_sock, wait_for_one=True)
    assert msg_bytes == recvd[0]


class TestSubMaster(unittest.TestCase):

  def test_init(self):
    sm = messaging.SubMaster(['carState'])
    sm.update()

if __name__ == "__main__":
  unittest.main()
