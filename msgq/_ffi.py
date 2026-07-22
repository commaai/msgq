import ctypes
import platform
from pathlib import Path


_SUFFIX = ".dylib" if platform.system() == "Darwin" else ".so"
_LIBRARY_PATH = Path(__file__).with_name(f"libmsgq_ffi{_SUFFIX}")

try:
  lib = ctypes.CDLL(str(_LIBRARY_PATH), use_errno=True)
except OSError as exc:
  raise ImportError(f"msgq native library is not built; run `scons` ({_LIBRARY_PATH})") from exc

lib.msgq_last_error.argtypes = []
lib.msgq_last_error.restype = ctypes.c_char_p


def native_error_message() -> str:
  message = lib.msgq_last_error()
  return message.decode("utf-8", errors="replace") if message else "native msgq call failed"
