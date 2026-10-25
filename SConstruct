import os
import platform
from pathlib import Path
import subprocess
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
  tools=["default"]
)

Export('env', 'arch', 'common')

envNanobind = env.Clone(LIBS=[])
nb_dir = Path(nanobind.source_dir()).parent
envNanobind.Append(CPPPATH=[nanobind.include_dir(), str(nb_dir / "ext/robin_map/include")],
                   CCFLAGS=["-fvisibility=hidden", "-fno-strict-aliasing"])
if arch == "Darwin":
  envNanobind["LINKFLAGS"] = ldflags + ["-bundle", "-undefined", "dynamic_lookup"]
else:
  envNanobind["LINKFLAGS"] = ldflags + ["-pthread", "-shared"]

# Third-party runtime sources do not follow our shadowing warning policy.
envNanobindRuntime = envNanobind.Clone()
envNanobindRuntime['CCFLAGS'] = [flag for flag in envNanobindRuntime['CCFLAGS'] if not flag.startswith('-Wshadow')]
nanobind_lib = envNanobindRuntime.SharedObject('#/build/nb_combined.os', str(nb_dir / "src/nb_combined.cpp"))
Export('envNanobind', 'nanobind_lib')

SConscript(['SConscript'])
