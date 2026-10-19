import os
from pathlib import Path
import subprocess
import sys

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext


class SConsBuildExt(build_ext):
  def build_extensions(self):
    subprocess.check_call([sys.executable, "-m", "SCons", "--minimal", "-j", str(self.parallel or os.cpu_count() or 1)])
    for ext in self.extensions:
      source = Path(*ext.name.split(".")).with_suffix(".so")
      target = Path(self.get_ext_fullpath(ext.name))
      target.parent.mkdir(parents=True, exist_ok=True)
      self.copy_file(str(source), str(target))


# nanobind supports the stable ABI on Python >= 3.12, so 3.11 gets its own wheel
abi3 = sys.version_info >= (3, 12)

setup(
  ext_modules=[
    Extension("msgq.ipc_pyx", sources=[], py_limited_api=abi3),
    Extension("msgq.visionipc.visionipc_pyx", sources=[], py_limited_api=abi3),
  ],
  cmdclass={"build_ext": SConsBuildExt},
  options={"bdist_wheel": {"py_limited_api": "cp312"}} if abi3 else {},
)
