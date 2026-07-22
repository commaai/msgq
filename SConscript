Import('env', 'common')


visionipc_dir = Dir('msgq/visionipc')

# Build msgq
msgq_objects = env.SharedObject([
  'msgq/ipc.cc',
  'msgq/event.cc',
  'msgq/impl_msgq.cc',
  'msgq/impl_fake.cc',
  'msgq/msgq.cc',
])
msgq = env.Library('msgq', msgq_objects)

# Build Vision IPC
vipc_files = ['visionipc.cc', 'visionipc_server.cc', 'visionipc_client.cc']
if File('/dev/ion').exists():
  vipc_files += ['visionbuf_ion.cc']
else:
  vipc_files += ['visionbuf.cc']
vipc_sources = [f'{visionipc_dir.abspath}/{f}' for f in vipc_files]

vipc_objects = env.SharedObject(vipc_sources)
visionipc = env.Library('visionipc', vipc_objects)


ffi_env = env.Clone()
ffi_env.AppendUnique(LINKFLAGS=['-pthread'])
msgq_ffi = ffi_env.SharedLibrary('msgq/msgq_ffi', ['msgq/ffi.cc', 'msgq/visionipc/ffi.cc'],
                                 LIBS=[visionipc, msgq]+common)

if GetOption('extras'):
  env.Program('msgq/test_runner', ['msgq/msgq_tests.cc'], LIBS=[msgq]+common)

Export('visionipc', 'msgq', 'msgq_ffi')
