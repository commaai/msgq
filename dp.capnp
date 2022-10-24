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
  dpLateralLanelines @6 :Bool;
  dpLateralCameraOffset @7 :Int8;
  dpLateralPathOffset @8 :Int8;
  dpIpAddr @9 :Text;
  dpUiTop @10 :Bool;
  dpUiSide @11 :Bool;
  dpUiVolume @12 :Int8;
  dpUiBrightness @13 :UInt8;
  dpUiDisplayMode @14 :UInt8;
  dpUiSpeed @15 :Bool;
  dpUiEvent @16 :Bool;
  dpUiFace @17 :Bool;
  dpUiLeadInfo @18 :Bool;
  dpToyotaSng @19 :Bool;
  dpAccelProfileCtrl @20 :Bool;
  dpAccelProfile @21 :UInt8;
  dpToyotaCruiseOverride @22 :Bool;
  dpToyotaCruiseOverrideSpeed @23 :UInt8;
  dpToyotaAutoLock @24 :Bool;
  dpToyotaAutoUnlock @25 :Bool;
  dpMapd @26 :Bool;
  dpLocalDb @27 :Bool;
  dpDashcamd @28 :Bool;
  dpMazdaSteerAlert @29 :Bool;
  dpSpeedCheck @30 :Bool;
  dpFollowingProfileCtrl @31 :Bool;
  dpFollowingProfile @32 :UInt8;
  dpE2EConditional @33 :Bool;
  dpE2EConditionalAtSpeed @34 :UInt8;
}
