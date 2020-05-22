from Cython.Build import cythonize
from distutils.core import Extension, setup  # pylint: disable=import-error,no-name-in-module


setup(ext_modules=cythonize(
  Extension(
    "log",
    sources=[f"log.pyx"],
    language="c++",
    extra_compile_args=["-std=c++14"],
    libraries=['kj', 'capnp'],
  )
))
