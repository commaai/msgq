using Cxx = import "./include/c++.capnp";
$Cxx.namespace("cereal");

@0xbfa7e645486440c7;

# dp
struct DragonConf {
  dpAtl @0 :UInt8;
  dpLocale @1 :Text;
  dpUiSpeed @2 :Bool;
  dpUiEvent @3 :Bool;
  dpUiMaxSpeed @4 :Bool;
  dpUiFace @5 :Bool;
  dpUiLane @6 :Bool;
  dpUiLead @7 :Bool;
  dpUiSide @8 :Bool;
  dpUiTop @9 :Bool;
  dpUiBlinker @10 :Bool;
  dpUiBrightness @11 :UInt8;
  dpUiVolume @12 :Int8;
  dpLateralMode @13 :UInt8;
  dpLcMinMph @14 :UInt8;
  dpLcAutoMinMph @15 :UInt8;
  dpLcAutoDelay @16 :Float32;
  dpIpAddr @17 :Text;
}
