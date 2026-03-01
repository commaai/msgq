import os
import numpy as np
from setuptools import setup
from Cython.Build import cythonize
from setuptools import Extension

msgq_dir = os.path.dirname(os.path.abspath(__file__))

# Common C++ compile args
cpp_args = ["-std=c++17", "-fPIC", "-O2"]

# msgq core C++ sources
msgq_cc_sources = [
    "msgq/ipc.cc",
    "msgq/event.cc",
    "msgq/impl_msgq.cc",
    "msgq/impl_fake.cc",
    "msgq/msgq.cc",
]

# visionipc C++ sources
vipc_cc_sources = [
    "msgq/visionipc/visionipc.cc",
    "msgq/visionipc/visionipc_server.cc",
    "msgq/visionipc/visionipc_client.cc",
    "msgq/visionipc/visionbuf.cc",
]

extensions = [
    Extension(
        "msgq.ipc_pyx",
        sources=["msgq/ipc_pyx.pyx"] + msgq_cc_sources,
        language="c++",
        extra_compile_args=cpp_args,
        include_dirs=[msgq_dir, np.get_include()],
    ),
    Extension(
        "msgq.visionipc.visionipc_pyx",
        sources=["msgq/visionipc/visionipc_pyx.pyx"] + vipc_cc_sources + msgq_cc_sources,
        language="c++",
        extra_compile_args=cpp_args,
        include_dirs=[msgq_dir, np.get_include()],
    ),
]

setup(ext_modules=cythonize(extensions, language_level="3"))
