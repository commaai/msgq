import sys
import sysconfig
from pathlib import Path

from cffi import FFI


kind = sys.argv[1]
root = Path(__file__).resolve().parent.parent
if kind == "ipc":
  package = root / "msgq"
  header = package / "ipc_cffi.h"
  module = "msgq._ipc_cffi_api"
  library = "ipc_cffi"
else:
  package = root / "msgq" / "visionipc"
  header = package / "visionipc_cffi.h"
  module = "msgq.visionipc._visionipc_cffi_api"
  library = "visionipc_cffi"

lines = header.read_text().splitlines()
declarations = "\n".join(line for line in lines if not line.startswith("#") and line not in ('extern "C" {', '}'))
builder = FFI()
builder.cdef(declarations)
runtime_library_dir = "@loader_path" if sys.platform == "darwin" else "$ORIGIN"
builder.set_source(module, f'#include "{header.relative_to(root)}"', include_dirs=[str(root)],
                   libraries=[library], library_dirs=[str(package)], runtime_library_dirs=[runtime_library_dir])
target = package / f"_{kind}_cffi_api{sysconfig.get_config_var('EXT_SUFFIX')}"
build_dir = root / "gen" / "cffi" / kind
build_dir.mkdir(parents=True, exist_ok=True)
builder.compile(tmpdir=str(build_dir), target=str(target), verbose=False)
