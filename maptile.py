"""This is an automatically generated stub for `maptile.capnp`."""
import os

import capnp  # type: ignore

capnp.remove_import_hook()
here = os.path.dirname(os.path.abspath(__file__))
module_file = os.path.abspath(os.path.join(here, "maptile.capnp"))
module = capnp.load(module_file)  # pylint: disable=no-member
Point = module.Point
PointBuilder = Point
PointReader = Point
PolyLine = module.PolyLine
PolyLineBuilder = PolyLine
PolyLineReader = PolyLine
Lane = module.Lane
LaneBuilder = Lane
LaneReader = Lane
TileSummary = module.TileSummary
TileSummaryBuilder = TileSummary
TileSummaryReader = TileSummary
MapTile = module.MapTile
MapTileBuilder = MapTile
MapTileReader = MapTile
