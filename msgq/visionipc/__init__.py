from msgq.visionipc.visionipc_pyx import VisionBuf, VisionIpcClient, VisionIpcServer, VisionStreamType, get_endpoint_name, set_logger
assert VisionBuf
assert VisionIpcClient
assert VisionIpcServer
assert VisionStreamType
assert get_endpoint_name
assert set_logger

try:
  import msgq
  set_logger(msgq._get_logger_callback())
except (AttributeError, ImportError):
  pass
