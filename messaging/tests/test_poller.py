import unittest
import time
import cereal.messaging as messaging

from multiprocessing import Process, Pipe


def poller(pipe):
  context = messaging.Context()

  sub = messaging.SubSocket()
  sub.connect(context, 'controlsState')

  p = messaging.Poller()
  p.registerSocket(sub)

  while True:
    pipe.recv()

    socks = p.poll(1000)
    pipe.send([s.receive(non_blocking=True) for s in socks])


class TestPoller(unittest.TestCase):
  def test_poll_once(self):
    context = messaging.Context()

    pub = messaging.PubSocket()
    pub.connect(context, 'controlsState')

    pipe, pipe_child = Pipe()
    proc = Process(target=poller, args=(pipe_child,))
    proc.start()

    time.sleep(.1)

    # Start poll
    pipe.send("go")

    # Send message
    pub.send("a")

    result = pipe.recv()
    proc.kill()

    self.assertEqual(result, [b"a"])
