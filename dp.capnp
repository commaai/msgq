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
  dpUiDmCam @21 :Bool;
  dpToyotaSng @22 :Bool;
  dpAccelProfileCtrl @23 :Bool;
  dpAccelProfile @24 :UInt8;
  dpToyotaCruiseOverride @25 :Bool;
  dpToyotaCruiseOverrideSpeed @26 :UInt8;
  dpToyotaAutoLock @27 :Bool;
  dpToyotaAutoUnlock @28 :Bool;
  dpMapd @29 :Bool;
  dpLocalDb @30 :Bool;
  dpDashcamd @31 :Bool;
  dpMazdaSteerAlert @32 :Bool;
  dpSpeedCheck @33 :Bool;
  dpFollowingProfileCtrl @34 :Bool;
  dpFollowingProfile @35 :UInt8;
  dpE2EConditional @36 :Bool;
  dpE2EConditionalAtSpeed @37 :UInt8;
  dpE2EConditionalAtSpeedLead @38 :UInt8;
  dpE2EConditionalAdaptFp @39 :Bool;
  dpE2EConditionalAdaptAp @40 :Bool;
  dpE2EConditionalVoacc @41 :Bool;
}
