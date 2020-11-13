import Cython
import distutils
import os
import subprocess
import sys
import numpy as np
from sysconfig import get_paths

zmq = 'zmq'
arch = subprocess.check_output(["uname", "-m"], encoding='utf8').rstrip()

python_path = get_paths()['include']
numpy_path = np.get_include()
# Rebuild cython extensions if python, distutils, or cython change
cython_dependencies = [Value(v) for v in (sys.version, distutils.__version__, Cython.__version__)]
Export('cython_dependencies')

cereal_dir = Dir('.')
cpppath = [
  cereal_dir,
  os.getcwd()+"/messaging",
  '/usr/lib/include',
  python_path, 
  numpy_path
]

AddOption('--test',
          action='store_true',
          help='build test files')

AddOption('--asan',
          action='store_true',
          help='turn on ASAN')

ccflags_asan = ["-fsanitize=address", "-fno-omit-frame-pointer"] if GetOption('asan') else []
ldflags_asan = ["-fsanitize=address"] if GetOption('asan') else []

env = Environment(
  ENV=os.environ,
  CC='clang',
  CXX='clang++',
  CCFLAGS=[
    "-g",
    "-fPIC",
    "-O2",
    "-Wunused",
    "-Werror",
  ] + ccflags_asan,
  LDFLAGS=ldflags_asan,
  LINKFLAGS=ldflags_asan,

  CFLAGS="-std=gnu11",
  CXXFLAGS="-std=c++1z",
  CPPPATH=cpppath,
  CYTHONCFILESUFFIX=".cpp",
  tools=["default", "cython"]
)
Export('env', 'zmq', 'arch')


envCython = env.Clone()
envCython["CPPPATH"] += [python_path, numpy_path]
envCython["CCFLAGS"] += ["-Wno-#warnings", "-Wno-deprecated-declarations"]

python_libs = []
if arch == "Darwin":
  envCython["LINKFLAGS"]=["-bundle", "-undefined", "dynamic_lookup"]
elif arch == "aarch64":
  envCython["LINKFLAGS"]=["-shared"]

  python_libs.append(os.path.basename(python_path))
else:
  envCython["LINKFLAGS"]=["-pthread", "-shared"]

envCython["LIBS"] = python_libs

Export('envCython')


SConscript(['SConscript'])
