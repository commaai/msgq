import os
import platform
import subprocess
import sysconfig
import numpy as np

arch = subprocess.check_output(["uname", "-m"], encoding='utf8').rstrip()
if platform.system() == "Darwin":
  target = "Darwin"
elif platform.system() == "Linux":
  target = f"linux-{arch}"
else:
  raise Exception("Unsupported platform: ", platform.system())

common = ''

cpppath = [
  f"#/../",
  '/usr/lib/include',
  '/opt/homebrew/include',
  sysconfig.get_paths()['include'],
]

libpath = [
  '/opt/homebrew/lib',
]

AddOption('--minimal',
          action='store_false',
          dest='extras',
          default=True,
          help='the minimum build. no tests, tools, etc.')

AddOption('--asan',
          action='store_true',
          help='turn on ASAN')

AddOption('--ubsan',
          action='store_true',
          help='turn on UBSan')

ccflags = []
ldflags = []
if GetOption('ubsan'):
  flags = [
    "-fsanitize=undefined",
    "-fno-sanitize-recover=undefined",
  ]
  ccflags += flags
  ldflags += flags
elif GetOption('asan'):
  ccflags += ["-fsanitize=address", "-fno-omit-frame-pointer"]
  ldflags += ["-fsanitize=address"]

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
    "-Wshadow",
  ] + ccflags,
  LDFLAGS=ldflags,
  LINKFLAGS=ldflags,

  CFLAGS="-std=gnu11",
  CXXFLAGS="-std=c++1z",
  CPPPATH=cpppath,
  LIBPATH=libpath,
  CYTHONCFILESUFFIX=".cpp",
  tools=["default", "cython"]
)

Export('env', 'target', 'arch', 'common')

envCython = env.Clone(LIBS=[])
envCython["CPPPATH"] += [np.get_include()]
envCython["CCFLAGS"] += ["-Wno-#warnings", "-Wno-shadow", "-Wno-deprecated-declarations"]
envCython["CCFLAGS"].remove('-Werror')
if target == "Darwin":
  envCython["LINKFLAGS"] = ["-bundle", "-undefined", "dynamic_lookup"]
else:
  envCython["LINKFLAGS"] = ["-pthread", "-shared"]

Export('envCython')


SConscript(['SConscript'])
