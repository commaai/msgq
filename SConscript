import sys
import sysconfig

Import('env', 'envNanobind', 'nanobind_lib', 'common')


visionipc_dir = Dir('msgq/visionipc')
python_suffix = sysconfig.get_config_var('EXT_SUFFIX')

# Build msgq
msgq_objects = env.SharedObject([
  'msgq/ipc.cc',
  'msgq/event.cc',
  'msgq/msgq.cc',
])
msgq = env.Library('msgq', msgq_objects)
# Keep the extension module names compatible with existing Python imports.
msgq_python = envNanobind.Program(f'msgq/ipc_pyx{python_suffix}', ['msgq/ipc_py.cc', nanobind_lib], LIBS=envNanobind["LIBS"]+[msgq]+common)

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
visionipc_python = envNanobind.Program(f'{visionipc_dir.abspath}/visionipc_pyx{python_suffix}', [f'{visionipc_dir.abspath}/visionipc_py.cc', nanobind_lib],
                  LIBS=vipc_libs)

# ASan modules require a preloaded runtime before Python can import them.
if not GetOption('asan'):
  for module in ('msgq.ipc_pyx', 'msgq.visionipc.visionipc_pyx'):
    env.Command(module.replace('.', '/') + '.pyi', [msgq_python, visionipc_python],
                f'"{sys.executable}" -m nanobind.stubgen -i "{Dir("#").abspath}" -m {module} -o "$TARGET"')

if GetOption('extras'):
  env.Program('msgq/test_runner', ['msgq/msgq_tests.cc'], LIBS=[msgq]+common)

Export('visionipc', 'msgq', 'msgq_python')
