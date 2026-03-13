#!/usr/bin/env python3
"""Build system for msgq — replaces scons with direct clang++ subprocess calls."""
import os
import platform
import subprocess
import sysconfig
from pathlib import Path

import numpy as np

ROOT = Path(__file__).parent.resolve()
MSGQ_DIR = ROOT / "msgq"
VIPC_DIR = MSGQ_DIR / "visionipc"

arch = subprocess.check_output(["uname", "-m"], encoding="utf8").strip()
IS_DARWIN = platform.system() == "Darwin"
if IS_DARWIN:
  arch = "Darwin"

CXX = "clang++"
AR = "ar"

EXTRA_CXXFLAGS = os.environ.get("EXTRA_CXXFLAGS", "").split()

CXXFLAGS = [
  "-std=c++1z", "-g", "-fPIC", "-O2",
  "-Wunused", "-Werror",
  "-Wshadow" if IS_DARWIN else "-Wshadow=local",
  "-Wno-vla-cxx-extension",
  "-Wno-unknown-warning-option",
  "-MMD",
] + EXTRA_CXXFLAGS

CYTHON_CXXFLAGS = [
  "-std=c++1z", "-g", "-fPIC", "-O2",
  "-Wno-#warnings", "-Wno-cpp", "-Wno-shadow",
  "-Wno-deprecated-declarations",
  "-Wno-unknown-warning-option",
]

INCLUDE = [
  f"-I{ROOT}",
  f"-I{MSGQ_DIR}",
  "-I/usr/lib/include",
  f"-I{sysconfig.get_paths()['include']}",
]

CYTHON_INCLUDE = INCLUDE + [f"-I{np.get_include()}"]

if IS_DARWIN:
  CYTHON_LDFLAGS = ["-bundle", "-undefined", "dynamic_lookup"]
else:
  CYTHON_LDFLAGS = ["-pthread", "-shared"]


def _parse_depfile(depfile: Path) -> list[str]:
  """Parse a .d dependency file and return list of dependency paths."""
  if not depfile.exists():
    return []
  text = depfile.read_text()
  # Format: "target: dep1 dep2 \\\n  dep3 dep4"
  text = text.replace("\\\n", " ")
  # Strip "target:" prefix
  _, _, deps = text.partition(":")
  return deps.split()


def _needs_rebuild(output: Path, sources: list[Path], depfile: Path | None = None) -> bool:
  """Check if output needs rebuilding based on source and dependency mtimes."""
  if not output.exists():
    return True
  out_mtime = output.stat().st_mtime
  for src in sources:
    if src.exists() and src.stat().st_mtime > out_mtime:
      return True
  if depfile is not None:
    for dep in _parse_depfile(depfile):
      p = Path(dep)
      if p.exists() and p.stat().st_mtime > out_mtime:
        return True
  return False


def _compile(src: Path, obj: Path, extra_flags: list[str] | None = None) -> bool:
  """Compile a .cc to .o. Returns True if compilation ran."""
  depfile = obj.with_suffix(".d")
  if not _needs_rebuild(obj, [src], depfile):
    return False
  cmd = [CXX] + CXXFLAGS + INCLUDE + (extra_flags or []) + ["-c", str(src), "-o", str(obj)]
  subprocess.check_call(cmd, cwd=ROOT)
  return True


def _compile_parallel(sources: list[Path], objects: list[Path]) -> bool:
  """Compile multiple .cc files in parallel. Returns True if any compiled."""
  jobs = [(src, obj) for src, obj in zip(sources, objects)
          if _needs_rebuild(obj, [src], obj.with_suffix(".d"))]
  if not jobs:
    return False
  procs = []
  for src, obj in jobs:
    cmd = [CXX] + CXXFLAGS + INCLUDE + ["-c", str(src), "-o", str(obj)]
    procs.append((src, subprocess.Popen(cmd, cwd=ROOT)))
  for src, proc in procs:
    if proc.wait() != 0:
      raise subprocess.CalledProcessError(proc.returncode, f"compile {src}")
  return True


def _archive(lib: Path, objects: list[Path]) -> bool:
  """Create a static library from objects. Returns True if archive ran."""
  if not _needs_rebuild(lib, objects):
    return False
  subprocess.check_call([AR, "rcs", str(lib)] + [str(o) for o in objects], cwd=ROOT)
  return True


def _cythonize(pyx: Path, cpp: Path) -> bool:
  """Cythonize a .pyx to .cpp. Returns True if cythonize ran."""
  # Check pyx and all .pxd files in the same directory
  pxd_files = list(pyx.parent.glob("*.pxd"))
  if not _needs_rebuild(cpp, [pyx] + pxd_files):
    return False
  subprocess.check_call(["cythonize", str(pyx)], cwd=ROOT)
  return True


def _compile_cython_so(cpp: Path, so: Path, libs: list[Path]) -> bool:
  """Compile a Cython .cpp into a .so. Returns True if compilation ran."""
  if not _needs_rebuild(so, [cpp] + libs):
    return False
  obj = cpp.with_suffix(".o")
  subprocess.check_call(
    [CXX] + CYTHON_CXXFLAGS + CYTHON_INCLUDE + ["-c", str(cpp), "-o", str(obj)],
    cwd=ROOT,
  )
  subprocess.check_call(
    [CXX] + CYTHON_LDFLAGS + [str(obj)] + [str(l) for l in libs] + ["-o", str(so)],
    cwd=ROOT,
  )
  return True


# --- msgq ---

MSGQ_SOURCES = [MSGQ_DIR / f for f in ["ipc.cc", "event.cc", "impl_msgq.cc", "impl_fake.cc", "msgq.cc"]]
MSGQ_OBJECTS = [s.with_suffix(".o") for s in MSGQ_SOURCES]
LIBMSGQ = ROOT / "libmsgq.a"


def build_msgq():
  """Build libmsgq.a and msgq/ipc_pyx.so."""
  any_compiled = _compile_parallel(MSGQ_SOURCES, MSGQ_OBJECTS)

  if any_compiled or not LIBMSGQ.exists():
    _archive(LIBMSGQ, MSGQ_OBJECTS)

  pyx = MSGQ_DIR / "ipc_pyx.pyx"
  cpp = MSGQ_DIR / "ipc_pyx.cpp"
  so = MSGQ_DIR / "ipc_pyx.so"
  _cythonize(pyx, cpp)
  _compile_cython_so(cpp, so, [LIBMSGQ])


# --- visionipc ---

VIPC_FILES = ["visionipc.cc", "visionipc_server.cc", "visionipc_client.cc"]
if arch == "larch64":
  VIPC_FILES.append("visionbuf_ion.cc")
else:
  VIPC_FILES.append("visionbuf.cc")

VIPC_SOURCES = [VIPC_DIR / f for f in VIPC_FILES]
VIPC_OBJECTS = [s.with_suffix(".o") for s in VIPC_SOURCES]
LIBVIPC = ROOT / "libvisionipc.a"


def build_visionipc():
  """Build libvisionipc.a and msgq/visionipc/visionipc_pyx.so."""
  build_msgq()

  any_compiled = _compile_parallel(VIPC_SOURCES, VIPC_OBJECTS)

  if any_compiled or not LIBVIPC.exists():
    _archive(LIBVIPC, VIPC_OBJECTS)

  pyx = VIPC_DIR / "visionipc_pyx.pyx"
  cpp = VIPC_DIR / "visionipc_pyx.cpp"
  so = VIPC_DIR / "visionipc_pyx.so"
  _cythonize(pyx, cpp)
  _compile_cython_so(cpp, so, [LIBVIPC, LIBMSGQ])


# --- test runners ---

def build_test_runners():
  """Build C++ test executables."""
  build_visionipc()

  # msgq test runner
  test_srcs = [MSGQ_DIR / "test_runner.cc", MSGQ_DIR / "msgq_tests.cc"]
  test_objs = [s.with_suffix(".o") for s in test_srcs]
  test_bin = MSGQ_DIR / "test_runner"
  _compile_parallel(test_srcs, test_objs)
  if _needs_rebuild(test_bin, test_objs + [LIBMSGQ]):
    subprocess.check_call(
      [CXX] + [str(o) for o in test_objs] + [str(LIBMSGQ), "-o", str(test_bin)],
      cwd=ROOT,
    )

  # visionipc test runner
  vipc_test_srcs = [VIPC_DIR / "test_runner.cc", VIPC_DIR / "visionipc_tests.cc"]
  vipc_test_objs = [s.with_suffix(".o") for s in vipc_test_srcs]
  vipc_test_bin = VIPC_DIR / "test_runner"
  _compile_parallel(vipc_test_srcs, vipc_test_objs)
  if _needs_rebuild(vipc_test_bin, vipc_test_objs + [LIBVIPC, LIBMSGQ]):
    subprocess.check_call(
      [CXX] + [str(o) for o in vipc_test_objs] + [str(LIBVIPC), str(LIBMSGQ), "-lpthread", "-o", str(vipc_test_bin)],
      cwd=ROOT,
    )


def build():
  """Build everything."""
  build_visionipc()


if __name__ == "__main__":
  build()
  build_test_runners()
