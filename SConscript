Import('env', 'envCython', 'arch', 'common')

import shutil

cereal_dir = Dir('.')
gen_dir = Dir('gen')
java_gen_dir = Dir('java/ai.flow.definitions')
messaging_dir = Dir('messaging')

# Build cereal

schema_files = ['log.capnp', 'car.capnp', 'legacy.capnp']
env.Command(["gen/c/include/c++.capnp.h"], [], "mkdir -p " + gen_dir.path + "/c/include && touch $TARGETS")
env.Command([f'gen/cpp/{s}.c++' for s in schema_files] + [f'gen/cpp/{s}.h' for s in schema_files],
            schema_files,
            f"capnpc --src-prefix={cereal_dir.path} $SOURCES -o c++:{gen_dir.path}/cpp/")
 
java_outs = ["Definitions.java", "CarDefinitions.java", "Legacy.java"]
env.Command([f'java/ai.flow.definitions/{out}' for out in java_outs],
            schema_files,
            f"capnpc --src-prefix={cereal_dir.path} $SOURCES -ojava:{java_gen_dir.path}")

# TODO: remove non shared cereal and messaging
cereal_objects = env.SharedObject([f'gen/cpp/{s}.c++' for s in schema_files])

env.Library('cereal', cereal_objects)
env.SharedLibrary('cereal_shared', cereal_objects)

# Build messaging

services_h = env.Command(['services.h'], ['services.py', 'resources/services.yaml'], 'python3 ' + cereal_dir.path + '/services.py > $TARGET')

messaging_objects = env.SharedObject([
  'messaging/messaging.cc',
  'messaging/impl_zmq.cc',
  'messaging/impl_msgq.cc',
  'messaging/msgq.cc',
  'messaging/socketmaster.cc',
])

messaging_lib = env.Library('messaging', messaging_objects)
Depends('messaging/impl_zmq.cc', services_h)

env.Program('messaging/bridge', ['messaging/bridge.cc'], LIBS=[messaging_lib, 'zmq', common])
Depends('messaging/bridge.cc', services_h)

envCython.Program('messaging/messaging_pyx.so', 'messaging/messaging_pyx.pyx', LIBS=envCython["LIBS"]+[messaging_lib, "zmq", common])


if GetOption('test'):
  env.Program('messaging/test_runner', ['messaging/test_runner.cc', 'messaging/msgq_tests.cc'], LIBS=[messaging_lib, common])

