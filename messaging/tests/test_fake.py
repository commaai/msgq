import os
import unittest
import multiprocessing
from parameterized import parameterized_class

import cereal.messaging as messaging

WAIT_TIMEOUT = 5


class TestEvents(unittest.TestCase):

  def test_mutation(self):
    manager = messaging.fake_event_manager("carState")
    event = manager.recv_called_event

    self.assertFalse(event.peek())
    event.set()
    self.assertTrue(event.peek())
    event.clear()
    self.assertFalse(event.peek())

    del event

  def test_wait(self):
    manager = messaging.fake_event_manager("carState")
    event = manager.recv_called_event

    event.set()
    try:
      event.wait(WAIT_TIMEOUT)
      self.assertTrue(event.peek())
    except RuntimeError:
      self.fail("event.wait() timed out")

  def test_wait_multiprocess(self):
    manager = messaging.fake_event_manager("carState")
    event = manager.recv_called_event

    def set_event_run():
      event.set()

    try:
      p = multiprocessing.Process(target=set_event_run)
      p.start()
      event.wait(WAIT_TIMEOUT)
      self.assertTrue(event.peek())
    except RuntimeError:
      self.fail("event.wait() timed out")

    p.kill()

  def test_wait_zero_timeout(self):
    manager = messaging.fake_event_manager("carState")
    event = manager.recv_called_event

    try:
      event.wait(0)
      self.fail("event.wait() did not time out")
    except RuntimeError:
      self.assertFalse(event.peek())


@unittest.skipIf("ZMQ" in os.environ, "FakeSockets not supported on ZMQ")
@parameterized_class([{"prefix": None}, {"prefix": "test"}])
class TestFakeSockets(unittest.TestCase):
  prefix = None

  def setUp(self):
    messaging.toggle_fake_events(True)
    if self.prefix is not None:
      messaging.set_fake_prefix(self.prefix)
    else:
      messaging.delete_fake_prefix()

  def tearDown(self):
    messaging.toggle_fake_events(False)
    messaging.delete_fake_prefix()

  def test_event_manager_init(self):
    manager = messaging.fake_event_manager("controlsState", override=True)

    self.assertFalse(manager.enabled)
    self.assertGreaterEqual(manager.recv_called_event.fd, 0)
    self.assertGreaterEqual(manager.recv_ready_event.fd, 0)

  def test_non_managed_socket_state(self):
    # non managed socket should have zero state
    _ = messaging.pub_sock("ubloxGnss")

    manager = messaging.fake_event_manager("ubloxGnss", override=False)

    self.assertFalse(manager.enabled)
    self.assertEqual(manager.recv_called_event.fd, 0)
    self.assertEqual(manager.recv_ready_event.fd, 0)

  def test_managed_socket_state(self):
    # managed socket should not change anything about the state
    manager = messaging.fake_event_manager("ubloxGnss")
    manager.enabled = True

    expected_enabled = manager.enabled
    expected_recv_called_fd = manager.recv_called_event.fd
    expected_recv_ready_fd = manager.recv_ready_event.fd

    _ = messaging.pub_sock("ubloxGnss")

    self.assertEqual(manager.enabled, expected_enabled)
    self.assertEqual(manager.recv_called_event.fd, expected_recv_called_fd)
    self.assertEqual(manager.recv_ready_event.fd, expected_recv_ready_fd)

  def test_sockets_enable_disable(self):
    carState_manager = messaging.fake_event_manager("ubloxGnss", enable=True)
    recv_called = carState_manager.recv_called_event
    recv_ready = carState_manager.recv_ready_event

    pub_sock = messaging.pub_sock("ubloxGnss")
    sub_sock = messaging.sub_sock("ubloxGnss")

    try:
      carState_manager.enabled = True
      recv_ready.set()
      pub_sock.send(b"test")
      _ = sub_sock.receive()
      self.assertTrue(recv_called.peek())
      recv_called.clear()

      carState_manager.enabled = False
      recv_ready.set()
      pub_sock.send(b"test")
      _ = sub_sock.receive()
      self.assertFalse(recv_called.peek())
    except RuntimeError:
      self.fail("event.wait() timed out")

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
    
    carState_manager = messaging.fake_event_manager("carState", enable=True)
    recv_called = carState_manager.recv_called_event
    recv_ready = carState_manager.recv_ready_event

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
