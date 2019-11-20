Import('env', 'arch', 'zmq')

gen_dir = Dir('gen')

# TODO: remove src-prefix and cereal from command string. can we set working directory?
env.Command(["gen/c/include/c++.capnp.h", "gen/c/include/java.capnp.h"], [], "mkdir -p " + gen_dir.path + "/c/include && touch $TARGETS")
env.Command(
  ['gen/c/car.capnp.c', 'gen/c/log.capnp.c', 'gen/c/car.capnp.h', 'gen/c/log.capnp.h'],
  ['car.capnp', 'log.capnp'],
  'capnpc $SOURCES --src-prefix=cereal -o c:' + gen_dir.path + '/c/')
env.Command(
  ['gen/cpp/car.capnp.c++', 'gen/cpp/log.capnp.c++', 'gen/cpp/car.capnp.h', 'gen/cpp/log.capnp.h'],
  ['car.capnp', 'log.capnp'],
  'capnpc $SOURCES --src-prefix=cereal -o c++:' + gen_dir.path + '/cpp/')

env.Library('cereal', [
    'gen/c/car.capnp.c',
    'gen/c/log.capnp.c',
    'gen/cpp/car.capnp.c++',
    'gen/cpp/log.capnp.c++',
  ])

env.Command(
  ['services.h'],
  ['service_list.yaml', 'services.py'],
  'python3 cereal/services.py > $TARGET')

messaging_deps = [
  'messaging/messaging.cc',
  'messaging/impl_zmq.cc',
  'messaging/impl_msgq.cc',
  'messaging/msgq.cc',
]

messaging_lib = env.Library('messaging', messaging_deps)

# note, this rebuilds the deps shared, zmq is statically linked to make APK happy
shared_lib_shared_lib = [zmq, 'm', 'stdc++']
if arch == "aarch64":
  shared_lib_shared_lib.append("gnustl_shared")
messaging_shared_lib = env.SharedLibrary('messaging_shared', messaging_deps, LIBS=shared_lib_shared_lib)
env.Command(['messaging/messaging.so'], [messaging_shared_lib], "chmod 777 $SOURCES && ln -sf `realpath $SOURCES` $TARGET")

env.Program('messaging/bridge', ['messaging/bridge.cc'], LIBS=['messaging', 'zmq'])

# different target?
#env.Program('messaging/demo', ['messaging/demo.cc'], LIBS=['messaging', 'zmq'])

env.Command(['messaging/messaging_pyx.so'],
  [messaging_lib, 'messaging/messaging_pyx_setup.py', 'messaging/messaging_pyx.pyx', 'messaging/messaging.pxd'],
  "cd cereal/messaging && python3 messaging_pyx_setup.py build_ext --inplace")
