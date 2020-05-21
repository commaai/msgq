from Cython.Build import cythonize
from distutils.core import Extension, setup  # pylint: disable=import-error,no-name-in-module


for e in ("car", "log"):
  setup(ext_modules=cythonize(
    Extension(
      e,
      sources=[f"{e}.pyx"],
      language="c++",
      extra_compile_args=["-std=c++14"],
      libraries=['kj', 'capnp'],
    )
  ))
