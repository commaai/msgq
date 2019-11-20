libpath = [
    "/usr/lib",
    "/usr/local/lib",
]

zmq = FindFile("libzmq.a", libpath)

env = Environment(
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
)

Export('env', 'zmq')
SConscript(['SConscript'])
