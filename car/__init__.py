"""This is an automatically generated stub for `car.capnp`."""
import os

import capnp  # type: ignore

capnp.remove_import_hook()
here = os.path.dirname(os.path.abspath(__file__))
module_file = os.path.abspath(os.path.join(here, "car.capnp"))
CarEvent = capnp.load(module_file).CarEvent
CarEventBuilder = CarEvent
CarEventReader = CarEvent
CarState = capnp.load(module_file).CarState
CarStateBuilder = CarState
CarStateReader = CarState
RadarData = capnp.load(module_file).RadarData
RadarDataBuilder = RadarData
RadarDataReader = RadarData
CarControl = capnp.load(module_file).CarControl
CarControlBuilder = CarControl
CarControlReader = CarControl
CarParams = capnp.load(module_file).CarParams
CarParamsBuilder = CarParams
CarParamsReader = CarParams