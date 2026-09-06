import msgq


subscriber = msgq.sub_sock("msgq_example", timeout=1000)

try:
  while True:
    message = subscriber.receive()
    if message is not None:
      print(f"Received: {message.decode('utf-8')}", flush=True)
except KeyboardInterrupt:
  pass
