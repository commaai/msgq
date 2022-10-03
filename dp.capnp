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
  dpUiChevronDist @15 :Bool;
  dpUiChevronSpeed @16 :Bool;
  dpToyotaSng @17 :Bool;
  dpAccelProfileCtrl @18 :Bool;
  dpAccelProfile @19 :UInt8;
  dpToyotaCruiseOverride @20 :Bool;
  dpToyotaCruiseOverrideSpeed @21 :UInt8;
  dpToyotaAutoLock @22 :Bool;
  dpToyotaAutoUnlock @23 :Bool;
  dpMapd @24 :Bool;
  dpLocalDb @25 :Bool;
  dpDashcamd @26 :Bool;
  dpMazdaSteerAlert @27 :Bool;
  dpSpeedCheck @28 :Bool;
  dpFollowingProfileCtrl @29 :Bool;
  dpFollowingProfile @30 :UInt8;
  dpE2EConditional @31 :Bool;
  dpE2EConditionalAtSpeed @32 :UInt8;
}
