using Cxx = import "./include/c++.capnp";
$Cxx.namespace("cereal");

using Java = import "./include/java.capnp";
$Java.package("ai.comma.openpilot.cereal");
$Java.outerClassname("Legacy");

@0x80ef1ec4889c2a63;

# legacy.capnp: a home for legacy and deprecated structs

# ********** Deprecated structs **********

struct LogRotate {
  segmentNum @0 :Int32;
  path @1 :Text;
}

struct LiveUI {
  rearViewCam @0 :Bool;
  alertText1 @1 :Text;
  alertText2 @2 :Text;
  awarenessStatus @3 :Float32;
}

struct OrbslamCorrection {
  correctionMonoTime @0 :UInt64;
  prePositionECEF @1 :List(Float64);
  postPositionECEF @2 :List(Float64);
  prePoseQuatECEF @3 :List(Float32);
  postPoseQuatECEF @4 :List(Float32);
  numInliers @5 :UInt32;
}

struct EthernetPacket {
  pkt @0 :Data;
  ts @1 :Float32;
}

struct CellInfo {
  timestamp @0 :UInt64;
  repr @1 :Text; # android toString() for now
}

struct WifiScan {
  bssid @0 :Text;
  ssid @1 :Text;
  capabilities @2 :Text;
  frequency @3 :Int32;
  level @4 :Int32;
  timestamp @5 :Int64;

  centerFreq0 @6 :Int32;
  centerFreq1 @7 :Int32;
  channelWidth @8 :ChannelWidth;
  operatorFriendlyName @9 :Text;
  venueName @10 :Text;
  is80211mcResponder @11 :Bool;
  passpoint @12 :Bool;

  distanceCm @13 :Int32;
  distanceSdCm @14 :Int32;

  enum ChannelWidth {
    w20Mhz @0;
    w40Mhz @1;
    w80Mhz @2;
    w160Mhz @3;
    w80Plus80Mhz @4;
  }
}

struct LiveEventData {
  name @0 :Text;
  value @1 :Int32;
}

struct ModelData {
  frameId @0 :UInt32;
  frameAge @12 :UInt32;
  frameDropPerc @13 :Float32;
  timestampEof @9 :UInt64;
  modelExecutionTime @14 :Float32;
  gpuExecutionTime @16 :Float32;
  rawPred @15 :Data;

  path @1 :PathData;
  leftLane @2 :PathData;
  rightLane @3 :PathData;
  lead @4 :LeadData;
  freePath @6 :List(Float32);

  settings @5 :ModelSettings;
  leadFuture @7 :LeadData;
  speed @8 :List(Float32);
  meta @10 :MetaData;
  longitudinal @11 :LongitudinalData;

  struct PathData {
    points @0 :List(Float32);
    prob @1 :Float32;
    std @2 :Float32;
    stds @3 :List(Float32);
    poly @4 :List(Float32);
    validLen @5 :Float32;
  }

  struct LeadData {
    dist @0 :Float32;
    prob @1 :Float32;
    std @2 :Float32;
    relVel @3 :Float32;
    relVelStd @4 :Float32;
    relY @5 :Float32;
    relYStd @6 :Float32;
    relA @7 :Float32;
    relAStd @8 :Float32;
  }

  struct ModelSettings {
    bigBoxX @0 :UInt16;
    bigBoxY @1 :UInt16;
    bigBoxWidth @2 :UInt16;
    bigBoxHeight @3 :UInt16;
    boxProjection @4 :List(Float32);
    yuvCorrection @5 :List(Float32);
    inputTransform @6 :List(Float32);
  }

  struct MetaData {
    engagedProb @0 :Float32;
    desirePrediction @1 :List(Float32);
    brakeDisengageProb @2 :Float32;
    gasDisengageProb @3 :Float32;
    steerOverrideProb @4 :Float32;
    desireState @5 :List(Float32);
  }

  struct LongitudinalData {
    distances @2 :List(Float32);
    speeds @0 :List(Float32);
    accelerations @1 :List(Float32);
  }
}


