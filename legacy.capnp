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

struct ECEFPoint @0xc25bbbd524983447 {
  x @0 :Float64;
  y @1 :Float64;
  z @2 :Float64;
}

struct ECEFPointDEPRECATED @0xe10e21168db0c7f7 {
  x @0 :Float32;
  y @1 :Float32;
  z @2 :Float32;
}

struct GPSPlannerPoints {
  curPosDEPRECATED @0 :ECEFPointDEPRECATED;
  pointsDEPRECATED @1 :List(ECEFPointDEPRECATED);
  curPos @6 :ECEFPoint;
  points @7 :List(ECEFPoint);
  valid @2 :Bool;
  trackName @3 :Text;
  speedLimit @4 :Float32;
  accelTarget @5 :Float32;
}

struct GPSPlannerPlan {
  valid @0 :Bool;
  poly @1 :List(Float32);
  trackName @2 :Text;
  speed @3 :Float32;
  acceleration @4 :Float32;
  pointsDEPRECATED @5 :List(ECEFPointDEPRECATED);
  points @6 :List(ECEFPoint);
  xLookahead @7 :Float32;
}

struct UiNavigationEvent {
  type @0: Type;
  status @1: Status;
  distanceTo @2: Float32;
  endRoadPointDEPRECATED @3: ECEFPointDEPRECATED;
  endRoadPoint @4: ECEFPoint;

  enum Type {
    none @0;
    laneChangeLeft @1;
    laneChangeRight @2;
    mergeLeft @3;
    mergeRight @4;
    turnLeft @5;
    turnRight @6;
  }

  enum Status {
    none @0;
    passive @1;
    approaching @2;
    active @3;
  }
}

struct OrbOdometry {
  # timing first
  startMonoTime @0 :UInt64;
  endMonoTime @1 :UInt64;

  # fundamental matrix and error
  f @2: List(Float64);
  err @3: Float64;

  # number of inlier points
  inliers @4: Int32;

  # for debug only
  # indexed by endMonoTime features
  # value is startMonoTime feature match
  # -1 if no match
  matches @5: List(Int16);
}

struct OrbFeatures {
  timestampEof @0 :UInt64;
  # transposed arrays of normalized image coordinates
  # len(xs) == len(ys) == len(descriptors) * 32
  xs @1 :List(Float32);
  ys @2 :List(Float32);
  descriptors @3 :Data;
  octaves @4 :List(Int8);

  # match index to last OrbFeatures
  # -1 if no match
  timestampLastEof @5 :UInt64;
  matches @6: List(Int16);
}

struct OrbFeaturesSummary {
  timestampEof @0 :UInt64;
  timestampLastEof @1 :UInt64;

  featureCount @2 :UInt16;
  matchCount @3 :UInt16;
  computeNs @4 :UInt64;
}

struct OrbKeyFrame {
  # this is a globally unique id for the KeyFrame
  id @0: UInt64;

  # this is the location of the KeyFrame
  pos @1: ECEFPoint;

  # these are the features in the world
  # len(dpos) == len(descriptors) * 32
  dpos @2 :List(ECEFPoint);
  descriptors @3 :Data;
}

struct KalmanOdometry {
  trans @0 :List(Float32); # m/s in device frame
  rot @1 :List(Float32); # rad/s in device frame
  transStd @2 :List(Float32); # std m/s in device frame
  rotStd @3 :List(Float32); # std rad/s in device frame
}

struct OrbObservation {
  observationMonoTime @0 :UInt64;
  normalizedCoordinates @1 :List(Float32);
  locationECEF @2 :List(Float64);
  matchDistance @3: UInt32;
}

struct CalibrationFeatures {
  frameId @0 :UInt32;

  p0 @1 :List(Float32);
  p1 @2 :List(Float32);
  status @3 :List(Int8);
}


