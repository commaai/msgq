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
  dpUiSide @7 :Bool;
  dpUiVolume @8 :Int8;
  dpUiBrightness @9 :UInt8;
  dpToyotaSng @10 :Bool;
  dpAccelProfileCtrl @11 :Bool;
  dpAccelProfile @12 :UInt8;
  dpUseLanelines @13 :Bool;
}
