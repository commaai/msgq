Import('env', 'envCython', 'arch', 'common')


visionipc_dir = Dir('msgq/visionipc')
gen_dir = Dir('gen')


# Build msgq
msgq_objects = env.SharedObject([
  'msgq/ipc.cc',
  'msgq/event.cc',
  'msgq/impl_zmq.cc',
  'msgq/impl_msgq.cc',
  'msgq/impl_fake.cc',
  'msgq/msgq.cc',
])
msgq = env.Library('msgq', msgq_objects)
msgq_c = env.Program('msgq/libmsgq_c.so', ['msgq/ipc_capi.cc'], LIBS=[msgq, "zmq", common], LINKFLAGS=['-shared'])

if GetOption('extras'):
  env.Program('msgq/test_runner', ['msgq/test_runner.cc', 'msgq/msgq_tests.cc'], LIBS=[msgq, common])

Export('msgq', 'msgq_c')
