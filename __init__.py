# pylint: skip-file
import os
import capnp

CEREAL_PATH = os.path.dirname(os.path.abspath(__file__))
capnp.remove_import_hook()

# ruff: noqa: F401
import cereal.gen.py.car_capnp as car
import cereal.gen.py.log_capnp as log
import cereal.gen.py.custom_capnp as custom
