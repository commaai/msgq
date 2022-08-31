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
  dpUiDisplayMode @11 :UInt8;
  dpUiSpeed @12 :Bool;
  dpUiEvent @13 :Bool;
  dpUiFace @14 :Bool;
  dpToyotaSng @15 :Bool;
  dpAccelProfileCtrl @16 :Bool;
  dpAccelProfile @17 :UInt8;
  dpToyotaCruiseOverride @18 :Bool;
  dpToyotaCruiseOverrideSpeed @19 :UInt8;
  dpToyotaAutoLock @20 :Bool;
  dpToyotaAutoUnlock @21 :Bool;
  dpUseLanelines @22 :Bool;
  dpMapd @23 :Bool;
  dpDashcamd @24 :Bool;
  dpMazdaSteerAlert @25 :Bool;
  dpCameraOffset @26 :Int8;
}
