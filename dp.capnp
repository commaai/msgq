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
  dpUiLaneline @19 :Bool;
  dpUiChevron @20 :Bool;
  dpToyotaSng @21 :Bool;
  dpAccelProfileCtrl @22 :Bool;
  dpAccelProfile @23 :UInt8;
  dpToyotaCruiseOverride @24 :Bool;
  dpToyotaCruiseOverrideSpeed @25 :UInt8;
  dpToyotaAutoLock @26 :Bool;
  dpToyotaAutoUnlock @27 :Bool;
  dpMapd @28 :Bool;
  dpLocalDb @29 :Bool;
  dpDashcamd @30 :Bool;
  dpMazdaSteerAlert @31 :Bool;
  dpSpeedCheck @32 :Bool;
  dpFollowingProfileCtrl @33 :Bool;
  dpFollowingProfile @34 :UInt8;
  dpE2EConditional @35 :Bool;
  dpE2EConditionalAtSpeed @36 :UInt8;
}
