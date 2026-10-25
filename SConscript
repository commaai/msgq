Import('env', 'envNanobind', 'nanobind_lib', 'common')


visionipc_dir = Dir('msgq/visionipc')

# Build msgq
msgq_objects = env.SharedObject([
  'msgq/ipc.cc',
  'msgq/event.cc',
  'msgq/msgq.cc',
])
msgq = env.Library('msgq', msgq_objects)
# Keep the extension module names compatible with existing Python imports.
msgq_python = envNanobind.Program('msgq/ipc_pyx.so', ['msgq/ipc_py.cc', nanobind_lib], LIBS=envNanobind["LIBS"]+[msgq]+common)

# Build Vision IPC
vipc_files = ['visionipc.cc', 'visionipc_server.cc', 'visionipc_client.cc']
if File('/dev/ion').exists():
  vipc_files += ['visionbuf_ion.cc']
else:
  vipc_files += ['visionbuf.cc']
vipc_sources = [f'{visionipc_dir.abspath}/{f}' for f in vipc_files]

vipc_objects = env.SharedObject(vipc_sources)
visionipc = env.Library('visionipc', vipc_objects)


vipc_libs = envNanobind["LIBS"] + [visionipc, msgq] + common
envNanobind.Program(f'{visionipc_dir.abspath}/visionipc_pyx.so', [f'{visionipc_dir.abspath}/visionipc_py.cc', nanobind_lib],
                  LIBS=vipc_libs)

if GetOption('extras'):
  env.Program('msgq/test_runner', ['msgq/msgq_tests.cc'], LIBS=[msgq]+common)

Export('visionipc', 'msgq', 'msgq_python')
