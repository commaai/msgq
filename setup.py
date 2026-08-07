from setuptools import Distribution, setup


class BinaryDistribution(Distribution):
  """Mark wheels containing the pre-built Cython modules as platform-specific."""

  def has_ext_modules(self) -> bool:
    return True


setup(distclass=BinaryDistribution)
