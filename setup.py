from setuptools import Command, setup
from setuptools.command.build import build
import subprocess
import os

class SconsBuild(Command):
  def initialize_options(self) -> None:
    pass

  def finalize_options(self) -> None:
    pass

  def run(self) -> None:
    scons_flags = '--minimal' if 'SCONS_EXTRAS' not in  os.environ else ''
    subprocess.run([f"scons {scons_flags} -j$(nproc)"], shell=True).check_returncode()

class CustomBuild(build):
  sub_commands = [('scons_build', None)] + build.sub_commands

setup(
    packages = ["msgq", "msgq.visionipc"],
    package_data={'msgq': ['**/*.cc', '**/*.h', '**/*.pxd', '**/*.pyx', '**/*.so']},
    include_package_data=True,
    cmdclass={'build': CustomBuild, 'scons_build': SconsBuild}
    )
