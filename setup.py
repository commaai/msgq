import os
import shlex
import shutil
import subprocess
from pathlib import Path

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext


ROOT = Path(__file__).resolve().parent
SCONS_OUTPUTS = {
  "msgq.ipc_pyx": ROOT / "msgq" / "ipc_pyx.so",
  "msgq.visionipc.visionipc_pyx": ROOT / "msgq" / "visionipc" / "visionipc_pyx.so",
}


class SConsBuildExt(build_ext):
  def run(self):
    self.run_scons()
    for ext in self.extensions:
      self.copy_scons_output(ext)

  def run_scons(self):
    scons = shutil.which("scons")
    if scons is None:
      raise RuntimeError("scons is required to build msgq extension modules")

    cmd = [scons, "--minimal"]
    if self.parallel:
      cmd += ["-j", str(self.parallel)]

    extra_args = os.environ.get("MSGQ_SCONS_ARGS")
    if extra_args:
      cmd += shlex.split(extra_args)

    self.announce(f"running {' '.join(cmd)}", level=2)
    subprocess.check_call(cmd, cwd=ROOT)

  def copy_scons_output(self, ext):
    source = SCONS_OUTPUTS[ext.name]
    if not source.exists():
      raise RuntimeError(f"expected SCons output was not built: {source}")

    destination = Path(self.get_ext_fullpath(ext.name))
    destination.parent.mkdir(parents=True, exist_ok=True)
    self.copy_file(str(source), str(destination))


setup(
  ext_modules=[
    Extension("msgq.ipc_pyx", sources=[]),
    Extension("msgq.visionipc.visionipc_pyx", sources=[]),
  ],
  cmdclass={"build_ext": SConsBuildExt},
)
