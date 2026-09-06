import time

import msgq


publisher = msgq.pub_sock("msgq_example")

try:
  while True:
    message = "Hello from MSGQ!"
    publisher.send(message.encode("utf-8"))
    print(f"Sent: {message}", flush=True)
    time.sleep(1)
except KeyboardInterrupt:
  pass
