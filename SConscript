Import('env', 'envCython', 'arch', 'common')


visionipc_dir = Dir('visionipc')
gen_dir = Dir('gen')


# Build Vision IPC
vipc_files = ['ipc.cc', 'visionipc_server.cc', 'visionipc_client.cc', 'visionbuf.cc']
vipc_sources = [f'{visionipc_dir.abspath}/{f}' for f in vipc_files]

if arch == "larch64":
  vipc_sources += [f'{visionipc_dir.abspath}/visionbuf_ion.cc']
else:
  vipc_sources += [f'{visionipc_dir.abspath}/visionbuf_cl.cc']

vipc_objects = env.SharedObject(vipc_sources)
visionipc = env.Library(visionipc_dir, vipc_objects)


vipc_frameworks = []
vipc_libs = envCython["LIBS"] + [visionipc, common, "zmq"]
if arch == "Darwin":
  vipc_frameworks.append('OpenCL')
else:
  vipc_libs.append('OpenCL')
envCython.Program(f'{visionipc_dir.abspath}/visionipc_pyx.so', f'{visionipc_dir.abspath}/visionipc_pyx.pyx',
                  LIBS=vipc_libs, FRAMEWORKS=vipc_frameworks)

if GetOption('extras'):
  env.Program(f'{visionipc_dir.abspath}/test_runner',
             [f'{visionipc_dir.abspath}/test_runner.cc', f'{visionipc_dir.abspath}/visionipc_tests.cc'],
              LIBS=['pthread'] + vipc_libs, FRAMEWORKS=vipc_frameworks)

Export('visionipc')
