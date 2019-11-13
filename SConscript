Import('env')

# TODO: remove src-prefix and cereal from command string. can we set working directory?
env.Command(["gen/c/include/c++.capnp.h", "gen/c/include/java.capnp.h"], [], "mkdir -p cereal/gen/c/include && touch $TARGETS")
env.Command(
  ['gen/c/car.capnp.c', 'gen/c/log.capnp.c'],
  ['car.capnp', 'log.capnp'],
  'capnpc $SOURCES --src-prefix=cereal -o c:cereal/gen/c/')
env.Command(
  ['gen/cpp/car.capnp.c++', 'gen/cpp/log.capnp.c++'],
  ['car.capnp', 'log.capnp'],
  'capnpc $SOURCES --src-prefix=cereal -o c++:cereal/gen/cpp/')

env.Library('cereal', [
    'gen/c/car.capnp.c',
    'gen/c/log.capnp.c',
    'gen/cpp/car.capnp.c++',
    'gen/cpp/log.capnp.c++',
  ])

env.Library('messaging', [
    'messaging/messaging.cc',
    'messaging/impl_zmq.cc',
    'messaging/impl_msgq.cc',
    'messaging/msgq.cc',
  ])

env.Program('messaging/bridge', ['messaging/bridge.cc'], LIBS=['messaging', 'yaml-cpp', 'zmq'])


