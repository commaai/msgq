from typing import TYPE_CHECKING, assert_type

if TYPE_CHECKING:
  import msgq
  from msgq.ipc_pyx import Event
  from msgq.visionipc import VisionBuf, VisionIpcClient, VisionIpcServer, get_endpoint_name

  publisher = msgq.pub_sock("typing")
  subscriber = msgq.sub_sock("typing")
  poller = msgq.Poller()
  poller.registerSocket(subscriber)
  assert_type(publisher.send(b"data"), None)
  assert_type(publisher.all_readers_updated(), bool)
  assert_type(subscriber.receive(), bytes | None)
  assert_type(poller.poll(0), list[msgq.SubSocket])
  assert_type(msgq.drain_sock_raw(subscriber), list[bytes])
  assert_type(msgq.get_fake_prefix(), bytes)

  handle = msgq.fake_event_handle("typing", identifier=b"prefix")
  handle.enabled = True
  assert_type(handle.enabled, bool)
  assert_type(handle.recv_called_event, Event)
  assert_type(handle.recv_ready_event.fd, int)
  assert_type(msgq.wait_for_one_event([handle.recv_called_event], 0), int)

  server = VisionIpcServer("typing")
  client = VisionIpcClient("typing", 0, False)
  assert_type(get_endpoint_name("typing", 0), str)
  assert_type(VisionIpcClient.available_streams("typing", False), set[int])
  assert_type(client.connect(False), bool)
  assert_type(client.width, int | None)
  assert_type(client.buffer_len, int | None)
  assert_type(client.recv(), VisionBuf | None)
  assert_type(client.frame_id, int)
  assert_type(client.valid, bool)
  frame = client.recv()
  if frame is not None:
    assert_type(frame.data, memoryview)
    assert_type(frame.width, int)
    assert_type(frame.frame_id, int)
    server.send(0, frame.data)
  server.send(0, b"frame")
  server.send(0, bytearray(b"frame"))
