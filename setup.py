from setuptools import Command, setup, Distribution
from setuptools.command.build import build
import subprocess
import os

class BinaryDistribution(Distribution):
  def has_ext_modules(self):
    return True


class SconsBuild(Command):
  def initialize_options(self) -> None:
    pass

  def finalize_options(self) -> None:
    pass

  def run(self) -> None:
    scons_flags = '' if 'BUILD_TESTS' in os.environ else '--minimal'
    subprocess.run([f"scons {scons_flags} -j$(nproc || sysctl -n hw.logicalcpu)"], shell=True).check_returncode()


class CustomBuild(build):
  sub_commands = [('scons_build', None)] + build.sub_commands

setup(
    packages = ["msgq", "msgq.visionipc"],
    package_data={'': ['**/*.cc', '**/*.c', '**/*.h', '**/*.pxd', '**/*.pyx', '**/*.py', '**/*.so', '**/*.npy']},
    include_package_data=True,
    cmdclass={'build': CustomBuild, 'scons_build': SconsBuild},
    distclass=BinaryDistribution,
    )
