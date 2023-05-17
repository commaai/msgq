import os
import unittest
import multiprocessing

import cereal.messaging as messaging

WAIT_TIMEOUT = 5


class TestEvents(unittest.TestCase):

  def test_mutation(self):
    event = messaging.fake_event("carState", messaging.EVENT_RECV_CALLED)

    self.assertFalse(event.peek())
    event.set()
    self.assertTrue(event.peek())
    event.clear()
    self.assertFalse(event.peek())

    del event

  def test_wait(self):
    event = messaging.fake_event("carState", messaging.EVENT_RECV_CALLED)

    event.set()
    try:
      event.wait(WAIT_TIMEOUT)
      self.assertTrue(event.peek())
    except:
      self.fail("event.wait() timed out")

  def test_wait_multiprocess(self):
    event = messaging.fake_event("carState", messaging.EVENT_RECV_CALLED)

    def set_event_run():
      event.set()

    try:
      p = multiprocessing.Process(target=set_event_run)
      p.start()
      event.wait(WAIT_TIMEOUT)
      self.assertTrue(event.peek())
    except:
      self.fail("event.wait() timed out")

    p.kill()

  def test_wait_zero_timeout(self):
    event = messaging.fake_event("carState", messaging.EVENT_RECV_CALLED)

    try:
      event.wait(0)
      self.fail("event.wait() did not time out")
    except:
      self.assertFalse(event.peek())


@unittest.skipIf("ZMQ" in os.environ, "FakeSockets not supported on ZMQ")
class TestFakeSockets(unittest.TestCase):

  def setUp(self):
    messaging.toggle_fake_events(True)

  def tearDown(self):
    messaging.toggle_fake_events(False)

  def test_synced_pub_sub(self):
    def daemon_repub_process_run():
      pub_sock = messaging.pub_sock("ubloxGnss")
      sub_sock = messaging.sub_sock("carState")

      frame = -1
      while True:
        frame += 1
        msg = sub_sock.receive(non_blocking=True)
        if msg is None:
          print("none received")
          continue

        bts = frame.to_bytes(8, 'little')
        pub_sock.send(bts)

    recv_called = messaging.fake_event("carState", messaging.EVENT_RECV_CALLED)
    recv_ready = messaging.fake_event("carState", messaging.EVENT_RECV_READY)

    p = multiprocessing.Process(target=daemon_repub_process_run)
    p.start()

    pub_sock = messaging.pub_sock("carState")
    sub_sock = messaging.sub_sock("ubloxGnss")

    try:
      for i in range(10):
        recv_called.wait(WAIT_TIMEOUT)
        recv_called.clear()

        if i == 0:
          sub_sock.receive(non_blocking=True)

        bts = i.to_bytes(8, 'little')
        pub_sock.send(bts)

        recv_ready.set()
        recv_called.wait(WAIT_TIMEOUT)

        msg = sub_sock.receive(non_blocking=True)
        self.assertIsNotNone(msg)
        self.assertEqual(len(msg), 8)

        frame = int.from_bytes(msg, 'little')
        self.assertEqual(frame, i)
    except RuntimeError:
      self.fail("event.wait() timed out")
    finally:
      p.kill()


if __name__ == "__main__":
  unittest.main()
