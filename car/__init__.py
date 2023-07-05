"""This is an automatically generated stub for `car.capnp`."""
import os

import capnp  # type: ignore

capnp.remove_import_hook()
here = os.path.dirname(os.path.abspath(__file__))
module_file = os.path.abspath(os.path.join(here, "car.capnp"))
module = capnp.load(module_file)
CarEvent = module.CarEvent
CarEventBuilder = CarEvent
CarEventReader = CarEvent
CarState = module.CarState
CarStateBuilder = CarState
CarStateReader = CarState
RadarData = module.RadarData
RadarDataBuilder = RadarData
RadarDataReader = RadarData
CarControl = module.CarControl
CarControlBuilder = CarControl
CarControlReader = CarControl
CarParams = module.CarParams
CarParamsBuilder = CarParams
CarParamsReader = CarParams
