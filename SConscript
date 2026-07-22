import sysconfig

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
ipc_cffi = env.SharedLibrary('msgq/libipc_cffi.so', ['msgq/ipc_cffi.cc'], LIBS=[msgq]+common)
cffi_suffix = sysconfig.get_config_var('EXT_SUFFIX')
ipc_cffi_api = env.Command(f'msgq/_ipc_cffi_api{cffi_suffix}', ['msgq/build_cffi.py', 'msgq/ipc_cffi.h'] + ipc_cffi,
                           f'$PYTHON -W ignore msgq/build_cffi.py ipc')
env.Clean(ipc_cffi_api, 'gen/cffi/ipc')
msgq_python = File('msgq/ipc_pyx.py')

# Build Vision IPC
vipc_files = ['visionipc.cc', 'visionipc_server.cc', 'visionipc_client.cc']
if File('/dev/ion').exists():
  vipc_files += ['visionbuf_ion.cc']
else:
  vipc_files += ['visionbuf.cc']
vipc_sources = [f'{visionipc_dir.abspath}/{f}' for f in vipc_files]

vipc_objects = env.SharedObject(vipc_sources)
visionipc = env.Library('visionipc', vipc_objects)


visionipc_cffi = env.SharedLibrary(f'{visionipc_dir.abspath}/libvisionipc_cffi.so', [f'{visionipc_dir.abspath}/visionipc_cffi.cc'],
                                  LIBS=[visionipc, msgq]+common)
visionipc_cffi_api = env.Command(f'{visionipc_dir.abspath}/_visionipc_cffi_api{cffi_suffix}',
                                 ['msgq/build_cffi.py', f'{visionipc_dir.abspath}/visionipc_cffi.h'] + visionipc_cffi,
                                 f'$PYTHON -W ignore msgq/build_cffi.py visionipc')
env.Clean(visionipc_cffi_api, 'gen/cffi/visionipc')

if GetOption('extras'):
  env.Program('msgq/test_runner', ['msgq/msgq_tests.cc'], LIBS=[msgq]+common)

Export('visionipc', 'msgq', 'msgq_python')
