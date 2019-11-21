import unittest
import os
import time
import cereal.messaging as messaging

import concurrent.futures


def poller():
  context = messaging.Context()

  p = messaging.Poller()

  sub = messaging.SubSocket()
  sub.connect(context, 'controlsState')
  p.registerSocket(sub)

  socks = p.poll(10000)
  r = [s.receive(non_blocking=True) for s in socks]

  return r


class TestPoller(unittest.TestCase):
  def test_poll_once(self):
    context = messaging.Context()

    pub = messaging.PubSocket()
    pub.connect(context, 'controlsState')

    with concurrent.futures.ThreadPoolExecutor() as e:
      poll = e.submit(poller)

      time.sleep(0.1)  # Slow joiner syndrome

      # Send message
      pub.send("a")

      # Wait for poll result
      result = poll.result()

    del pub
    context.term()

    self.assertEqual(result, [b"a"])

  def test_poll_and_create_many_subscribers(self):
    context = messaging.Context()

    pub = messaging.PubSocket()
    pub.connect(context, 'controlsState')

    with concurrent.futures.ThreadPoolExecutor() as e:
      poll = e.submit(poller)

      time.sleep(0.1)  # Slow joiner syndrome
      c = messaging.Context()
      for _ in range(10):
        messaging.SubSocket().connect(c, 'controlsState')

      # Send message
      pub.send("a")

      # Wait for poll result
      result = poll.result()

    del pub
    context.term()

    self.assertEqual(result, [b"a"])


if __name__ == "__main__":
  unittest.main()
