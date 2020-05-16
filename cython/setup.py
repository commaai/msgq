from Cython.Build import cythonize
from distutils.core import Extension, setup  # pylint: disable=import-error,no-name-in-module

setup(ext_modules=cythonize(
  Extension(
    "car",
    sources=["car.pyx"],
    language="c++",
    extra_compile_args=["-std=c++14"],
    libraries=['kj', 'capnp'],
  )
))
