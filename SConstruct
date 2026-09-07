import os
import platform
import subprocess
import sys
import sysconfig

WINDOWS = platform.system() == "Windows"
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
  CYTHONCFILESUFFIX=".cpp",
  tools=["mingw" if WINDOWS else "default", "cython"],  # the default tool picks MSVC on Windows
)
if WINDOWS:
  env["CC"], env["CXX"] = "clang", "clang++"  # the mingw tool assumes gcc
  env.Append(LINKFLAGS=["-static"])  # libc++ into the binaries so they run outside the MSYS2 shell
  common = ["ws2_32"]  # visionipc's sockets

Export('env', 'arch', 'common')

envCython = env.Clone(LIBS=[])
envCython["CCFLAGS"] += ["-Wno-#warnings", "-Wno-cpp", "-Wno-shadow", "-Wno-deprecated-declarations"]
envCython["CCFLAGS"].remove('-Werror')
if arch == "Darwin":
  envCython["LINKFLAGS"] = ["-bundle", "-undefined", "dynamic_lookup"]
elif WINDOWS:
  envCython["LINKFLAGS"] = ["-shared", "-static"]
  envCython.Append(LIBPATH=[os.path.join(sys.base_prefix, "libs")])
  envCython["LIBS"] = [f"python{sys.version_info.major}{sys.version_info.minor}"]
else:
  envCython["LINKFLAGS"] = ["-pthread", "-shared"]

Export('envCython')


SConscript(['SConscript'])
