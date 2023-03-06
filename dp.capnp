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
  dpUiBrightness @12 :UInt8;
  dpUiDisplayMode @13 :UInt8;
  dpUiSpeed @14 :Bool;
  dpUiEvent @15 :Bool;
  dpUiFace @16 :Bool;
  dpUiLeadInfo @17 :Bool;
  dpUiLaneline @18 :Bool;
  dpUiChevron @19 :Bool;
  dpUiDmCam @20 :Bool;
  dpUiRainbow @21 :Bool;
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
  dpLateralAlt @36 :Bool;
  dpLateralAltSpeed @37 :UInt8;
  dpLateralAltCtrl @38 :UInt8;
  dpLateralAltLanelines @39 :Bool;
  dpLateralAltCameraOffset @40 :Int8;
  dpLateralAltPathOffset @41 :Int8;
  dpE2EConditional @42 :Bool;
  dpE2EConditionalAdaptFp @43 :Bool;
  dpE2EConditionalAdaptAp @44 :Bool;
  dpE2EConditionalVoacc @45 :Bool;
}
