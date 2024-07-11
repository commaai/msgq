from setuptools import Command, setup
from setuptools.command.build import build
import subprocess

class SconsBuild(Command):
  def initialize_options(self) -> None:
    pass

  def finalize_options(self) -> None:
    pass

  def run(self) -> None:
    subprocess.run(["scons --minimal -j$(nproc)"], shell=True)

class CustomBuild(build):
  sub_commands = [('scons_build', None)] + build.sub_commands

setup(
    package_data={'msgq': ['**/*.cc', '**/*.h', '**/*.pxd', '**/*.pyx', '**/*.so']},
    cmdclass={'build': CustomBuild, 'scons_build': SconsBuild}
    )
