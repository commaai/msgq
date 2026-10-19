import os
import platform
import subprocess
import sys
import sysconfig

import nanobind

arch = subprocess.check_output(["uname", "-m"], encoding='utf8').rstrip()
if platform.system() == "Darwin":
  arch = "Darwin"

# shm_open/shm_unlink live in librt on older glibc versions (including manylinux).
common = [] if arch == "Darwin" else ["rt"]

cpppath = [
  "#/",
  '#msgq/',
  '/usr/lib/include',
  sysconfig.get_paths()['include'],
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
  CCFLAGS=[
    "-g",
    "-fPIC",
    "-O2",
    "-Wunused",
    "-Werror",
    "-Wshadow" if arch == "Darwin" else "-Wshadow=local",
    "-Wno-vla-cxx-extension",
    "-Wno-unknown-warning-option",
  ] + ccflags,
  LDFLAGS=ldflags,
  LINKFLAGS=ldflags,

  CFLAGS="-std=gnu11",
  CXXFLAGS="-std=c++1z",
  CPPPATH=cpppath,
)

Export('env', 'arch', 'common')

envPython = env.Clone(LIBS=[])
envPython["CCFLAGS"] += ["-fvisibility=hidden", "-isystem", nanobind.include_dir()]
envPython["CPPDEFINES"] = ["NB_COMPACT_ASSERTIONS"]
# On Python >= 3.12, build against the limited API so one build works on every newer Python
if sys.version_info >= (3, 12):
  envPython["CPPDEFINES"] += [("Py_LIMITED_API", "0x030C0000")]
if arch == "Darwin":
  envPython["LINKFLAGS"] = ["-bundle", "-undefined", "dynamic_lookup"]
else:
  envPython["LINKFLAGS"] = ["-pthread", "-shared"]

# nanobind's runtime, shared by both extension modules
envNanobind = envPython.Clone()
envNanobind["CCFLAGS"] = [f for f in envNanobind["CCFLAGS"] if f != "-Werror" and not f.startswith("-Wshadow")]
envNanobind["CCFLAGS"] += ["-fno-strict-aliasing", "-isystem", f"{nanobind.source_dir()}/../ext/robin_map/include"]
envNanobind["CPPDEFINES"] += ["NB_BUILD"]
nanobind_lib = envNanobind.Library('nanobind', envNanobind.SharedObject('nanobind', f"{nanobind.source_dir()}/nb_combined.cpp"))

Export('envPython', 'nanobind_lib')


SConscript(['SConscript'])
