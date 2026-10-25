import gc
import threading
import unittest

import msgq
from msgq.ipc_pyx import Event
from msgq.tests.test_messaging import random_sock


class TestBindings(unittest.TestCase):
  def test_bytes_endpoints_and_binary_messages(self):
    endpoint = random_sock().encode()
    publisher = msgq.PubSocket()
    subscriber = msgq.SubSocket()
    publisher.connect(endpoint)
    subscriber.connect(endpoint)
    publisher.send(b"\x00\xffpayload\x00")
    self.assertEqual(subscriber.receive(), b"\x00\xffpayload\x00")
    with self.assertRaises(TypeError):
      getattr(publisher, "send")("text")

  def test_fake_prefix_bytes(self):
    try:
      for prefix in ("prefix", b"prefix"):
        msgq.set_fake_prefix(prefix)
        self.assertEqual(msgq.get_fake_prefix(), prefix.encode() if isinstance(prefix, str) else prefix)
    finally:
      msgq.delete_fake_prefix()

  def test_event_keeps_handle_alive(self):
    event = msgq.fake_event_handle(random_sock()).recv_called_event
    gc.collect()
    event.set()
    self.assertTrue(event.peek())
    event.clear()
    self.assertFalse(event.peek())

  def test_event_wait_releases_gil(self):
    handle = msgq.fake_event_handle(random_sock())
    event = handle.recv_called_event
    for wait in (lambda: event.wait(1), lambda: msgq.wait_for_one_event([event], 1)):
      with self.subTest(wait=wait):
        event.clear()
        timer = threading.Timer(0.01, event.set)
        timer.start()
        try:
          wait()
          self.assertTrue(event.peek())
        finally:
          timer.join()

  def test_poller_socket_lifetime(self):
    endpoint = random_sock()
    publisher = msgq.pub_sock(endpoint)
    poller = msgq.Poller()
    subscriber = msgq.sub_sock(endpoint, poller=poller)
    del subscriber
    gc.collect()
    publisher.send(b"message")
    sockets = poller.poll(1000)
    self.assertEqual(len(sockets), 1)
    del poller
    gc.collect()
    self.assertEqual(sockets[0].receive(), b"message")

  def test_reader_timeout(self):
    publisher = msgq.pub_sock(random_sock())
    with self.assertRaisesRegex(TimeoutError, "subscriber did not connect"):
      publisher.wait_for_readers(timeout=0)

  def test_invalid_event(self):
    with self.assertRaisesRegex(RuntimeError, "valid file descriptor"):
      Event().wait(0)
