import os
import random
import time
import string
import sys
import pytest
import msgq


def random_sock():
  return ''.join(random.choices(string.ascii_uppercase + string.digits, k=10))

def random_bytes(length=1000):
  return bytes([random.randrange(0xFF) for _ in range(length)])

def zmq_sleep(t=1):
  if True or "ZMQ" in os.environ or sys.platform == "darwin":
    time.sleep(t)

class TestPubSubSockets:

  def setup_method(self):
    # ZMQ pub socket takes too long to die
    # sleep to prevent multiple publishers error between tests
    zmq_sleep()
    self.sockets = []

  def teardown_method(self):
    for sock in self.sockets:
      sock.close()
    self.sockets = []

  def _track(self, sock):
    self.sockets.append(sock)
    return sock

  def test_pub_sub(self):
    sock = random_sock()
    pub_sock = self._track(msgq.pub_sock(sock))
    sub_sock = self._track(msgq.sub_sock(sock, conflate=False, timeout=None))
    zmq_sleep(3)

    for _ in range(1000):
      msg = random_bytes()
      pub_sock.send(msg)
      recvd = sub_sock.receive()
      assert msg == recvd

  def test_conflate(self):
    sock = random_sock()
    pub_sock = self._track(msgq.pub_sock(sock))
    for conflate in [True, False]:
      for _ in range(10):
        num_msgs = random.randint(3, 10)
        sub_sock = self._track(msgq.sub_sock(sock, conflate=conflate, timeout=None))
        zmq_sleep()

        sent_msgs = []
        for __ in range(num_msgs):
          msg = random_bytes()
          pub_sock.send(msg)
          sent_msgs.append(msg)
        time.sleep(0.5)
        recvd_msgs = msgq.drain_sock_raw(sub_sock)
        if conflate:
          assert len(recvd_msgs) == 1
          assert recvd_msgs[0] == sent_msgs[-1]
        else:
          assert len(recvd_msgs) == len(sent_msgs)
          for rec_msg, sent_msg in zip(recvd_msgs, sent_msgs):
            assert rec_msg == sent_msg

  def test_receive_timeout(self):
    sock = random_sock()
    for _ in range(10):
      timeout = random.randrange(200)
      sub_sock = self._track(msgq.sub_sock(sock, timeout=timeout))
      zmq_sleep()

      start_time = time.monotonic()
      recvd = sub_sock.receive()
      assert (time.monotonic() - start_time) < 0.2
      assert recvd is None


class TestPoller:
  def test_poll(self):
    sock = random_sock()
    pub_sock = msgq.pub_sock(sock)
    sub_sock = msgq.sub_sock(sock, conflate=False, timeout=None)
    zmq_sleep()

    poller = msgq.Poller()
    poller.registerSocket(sub_sock)

    socks = poller.poll(100)
    assert len(socks) == 0

    msg = random_bytes()
    pub_sock.send(msg)
    time.sleep(0.1)

    socks = poller.poll(1000)
    assert len(socks) == 1
    assert socks[0] == sub_sock

    recvd = sub_sock.receive()
    assert recvd == msg


class TestEvents:
  @pytest.mark.skipif(sys.platform == "darwin", reason="SocketEventHandle not supported on macOS")
  def test_event_wait(self):
    e = msgq.Event()
    assert not e.peek()

    start_time = time.monotonic()
    e.wait(100)
    assert (time.monotonic() - start_time) >= 0.1

    e.set()
    assert e.peek()

    start_time = time.monotonic()
    e.wait(1000)
    assert (time.monotonic() - start_time) < 0.1

    e.clear()
    assert not e.peek()

  @pytest.mark.skipif(sys.platform == "darwin", reason="SocketEventHandle not supported on macOS")
  def test_socket_event_handle(self):
    h = msgq.SocketEventHandle("E", "I", False)
    assert not h.enabled
    h.enabled = True
    assert h.enabled

    assert isinstance(h.recv_called_event, msgq.Event)
    assert isinstance(h.recv_ready_event, msgq.Event)

