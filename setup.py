import os
from pathlib import Path
import shutil
import subprocess
import sys

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext


class SConsBuildExt(build_ext):
  def build_extensions(self):
    build_dir = Path(self.build_temp).resolve()
    build_dir.mkdir(parents=True, exist_ok=True)
    for path in ("SConstruct", "SConscript"):
      shutil.copy2(path, build_dir / path)
    for path in ("msgq", "site_scons"):
      shutil.copytree(path, build_dir / path, dirs_exist_ok=True,
                      ignore=shutil.ignore_patterns("*.so", "*.o", "*.os", "*.a", "*.cpp", "__pycache__"))
    subprocess.check_call([sys.executable, "-m", "SCons", "--minimal", "-j", str(self.parallel or os.cpu_count() or 1)], cwd=build_dir)
    for ext in self.extensions:
      source = build_dir.joinpath(*ext.name.split(".")).with_suffix(".so")
      target = Path(self.get_ext_fullpath(ext.name))
      target.parent.mkdir(parents=True, exist_ok=True)
      self.copy_file(str(source), str(target))


setup(
  ext_modules=[
    Extension("msgq.ipc_pyx", sources=[]),
    Extension("msgq.visionipc.visionipc_pyx", sources=[]),
  ],
  cmdclass={"build_ext": SConsBuildExt},
)
