import ctypes
import platform
from pathlib import Path


_SUFFIX = ".dylib" if platform.system() == "Darwin" else ".so"
_LIBRARY_PATH = Path(__file__).with_name(f"libmsgq_ffi{_SUFFIX}")

try:
  lib = ctypes.CDLL(str(_LIBRARY_PATH), use_errno=True)
except OSError as exc:
  raise ImportError(f"msgq native library is not built; run `scons` ({_LIBRARY_PATH})") from exc

def bind_raw(name, args, result=None):
  function = getattr(lib, name)
  function.argtypes = args
  function.restype = result
  return function


msgq_last_error = bind_raw("msgq_last_error", [], ctypes.c_char_p)


def bind(name, args, result=None):
  function = bind_raw(name, args, result)

  def checked(*call_args):
    value = function(*call_args)
    if error := msgq_last_error():
      raise RuntimeError(error.decode("utf-8", errors="replace"))
    return value

  return checked
