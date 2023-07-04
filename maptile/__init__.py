"""This is an automatically generated stub for `maptile.capnp`."""
import os

import capnp  # type: ignore

capnp.remove_import_hook()
here = os.path.dirname(os.path.abspath(__file__))
module_file = os.path.abspath(os.path.join(here, "maptile.capnp"))
Point = capnp.load(module_file).Point
PointBuilder = Point
PointReader = Point
PolyLine = capnp.load(module_file).PolyLine
PolyLineBuilder = PolyLine
PolyLineReader = PolyLine
Lane = capnp.load(module_file).Lane
LaneBuilder = Lane
LaneReader = Lane
TileSummary = capnp.load(module_file).TileSummary
TileSummaryBuilder = TileSummary
TileSummaryReader = TileSummary
MapTile = capnp.load(module_file).MapTile
MapTileBuilder = MapTile
MapTileReader = MapTile