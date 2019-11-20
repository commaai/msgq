import os

zmq = File("/lib/x86_64-linux-gnu/libzmq.a")

cereal_dir = Dir('.')

cpppath = [
    cereal_dir,
    '/usr/lib/include',
]

env = Environment(
  ENV=os.environ,
  CC='clang',
  CXX='clang++',
  CCFLAGS=[
    #"-g",
    "-fPIC",
    "-O2",
    "-Werror=implicit-function-declaration",
    "-Werror=incompatible-pointer-types",
    "-Werror=int-conversion",
    "-Werror=return-type",
    "-Werror=format-extra-args",
  ],
  CFLAGS="-std=gnu11",
  CXXFLAGS="-std=c++14",
  CPPPATH=cpppath,
)

AddOption('--test',
          action='store_true',
          help='build test files')

Export('env', 'zmq')
SConscript(['SConscript'])
