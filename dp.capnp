using Cxx = import "./include/c++.capnp";
$Cxx.namespace("cereal");

@0xbfa7e645486440c7;

# dp
struct DragonConf {
  dpAtl @0 :UInt8;
  dpLocale @1 :Text;
  dpLateralMode @2 :UInt8;
  dpLcMinMph @3 :UInt8;
  dpLcAutoMinMph @4 :UInt8;
  dpLcAutoDelay @5 :Float32;
  dpIpAddr @6 :Text;
  dpUiTop @7 :Bool;
  dpUiSide @8 :Bool;
  dpUiVolume @9 :Int8;
  dpUiBrightness @10 :UInt8;
  dpToyotaSng @11 :Bool;
  dpAccelProfileCtrl @12 :Bool;
  dpAccelProfile @13 :UInt8;
  dpUseLanelines @14 :Bool;
}
