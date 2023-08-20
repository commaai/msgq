# ruff: noqa: A003, F821, E501
"""This is an automatically generated stub for `car.capnp`."""
from __future__ import annotations

from contextlib import contextmanager
from io import BufferedWriter
from typing import Iterator, Literal, Sequence, overload

class CarEvent:
    EventName = Literal[
        "canError",
        "steerUnavailable",
        "brakeUnavailableDEPRECATED",
        "gasUnavailableDEPRECATED",
        "wrongGear",
        "doorOpen",
        "seatbeltNotLatched",
        "espDisabled",
        "wrongCarMode",
        "steerTempUnavailable",
        "reverseGear",
        "buttonCancel",
        "buttonEnable",
        "pedalPressed",
        "cruiseDisabled",
        "radarCanErrorDEPRECATED",
        "dataNeededDEPRECATED",
        "speedTooLow",
        "outOfSpace",
        "overheat",
        "calibrationIncomplete",
        "calibrationInvalid",
        "controlsMismatch",
        "pcmEnable",
        "pcmDisable",
        "noTargetDEPRECATED",
        "radarFault",
        "modelCommIssueDEPRECATED",
        "brakeHold",
        "parkBrake",
        "manualRestart",
        "lowSpeedLockout",
        "plannerError",
        "ipasOverrideDEPRECATED",
        "joystickDebug",
        "steerTempUnavailableSilent",
        "resumeRequired",
        "preDriverDistracted",
        "promptDriverDistracted",
        "driverDistracted",
        "geofenceDEPRECATED",
        "driverMonitorOnDEPRECATED",
        "driverMonitorOffDEPRECATED",
        "preDriverUnresponsive",
        "promptDriverUnresponsive",
        "driverUnresponsive",
        "belowSteerSpeed",
        "calibrationProgressDEPRECATED",
        "lowBattery",
        "invalidGiraffeHondaDEPRECATED",
        "vehicleModelInvalid",
        "accFaulted",
        "sensorDataInvalid",
        "commIssue",
        "tooDistracted",
        "posenetInvalid",
        "soundsUnavailable",
        "preLaneChangeLeft",
        "preLaneChangeRight",
        "laneChange",
        "invalidGiraffeToyotaDEPRECATED",
        "internetConnectivityNeededDEPRECATED",
        "communityFeatureDisallowedDEPRECATED",
        "lowMemory",
        "stockAeb",
        "ldw",
        "carUnrecognized",
        "radarCommIssueDEPRECATED",
        "driverMonitorLowAccDEPRECATED",
        "invalidLkasSetting",
        "speedTooHigh",
        "laneChangeBlocked",
        "relayMalfunction",
        "preEnableStandstill",
        "stockFcw",
        "startup",
        "startupNoCar",
        "startupNoControl",
        "startupMaster",
        "fcw",
        "steerSaturated",
        "whitePandaUnsupportedDEPRECATED",
        "startupOneplusDEPRECATED",
        "commIssueWarningDEPRECATED",
        "belowEngageSpeed",
        "noGps",
        "focusRecoverActiveDEPRECATED",
        "wrongCruiseMode",
        "neosUpdateRequiredDEPRECATED",
        "modeldLagging",
        "deviceFalling",
        "fanMalfunction",
        "cameraMalfunction",
        "modelLagWarningDEPRECATED",
        "gpsMalfunction",
        "processNotRunning",
        "dashcamMode",
        "startupFuzzyFingerprintDEPRECATED",
        "controlsInitializing",
        "usbError",
        "roadCameraError",
        "driverCameraError",
        "wideRoadCameraError",
        "localizerMalfunction",
        "startupNoFw",
        "highCpuUsage",
        "cruiseMismatch",
        "lkasDisabled",
        "gasPressedOverride",
        "commIssueAvgFreq",
        "cameraFrameRate",
        "canBusMissing",
        "controlsdLagging",
        "resumeBlocked",
        "steerOverride",
        "steerTimeLimit",
        "vehicleSensorsInvalid",
        "calibrationRecalibrating",
    ]
    name: CarEvent.EventName
    enable: bool
    noEntry: bool
    warning: bool
    userDisable: bool
    softDisable: bool
    immediateDisable: bool
    preEnable: bool
    permanent: bool
    overrideLongitudinal: bool
    overrideLateral: bool
    @staticmethod
    @contextmanager
    def from_bytes(
        data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
    ) -> Iterator[CarEventReader]: ...
    @staticmethod
    def from_bytes_packed(
        data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
    ) -> CarEventReader: ...
    @staticmethod
    def new_message() -> CarEventBuilder: ...
    def to_dict(self) -> dict: ...

class CarEventReader(CarEvent):
    def as_builder(self) -> CarEventBuilder: ...

class CarEventBuilder(CarEvent):
    @staticmethod
    def from_dict(dictionary: dict) -> CarEventBuilder: ...
    def copy(self) -> CarEventBuilder: ...
    def to_bytes(self) -> bytes: ...
    def to_bytes_packed(self) -> bytes: ...
    def to_segments(self) -> list[bytes]: ...
    def as_reader(self) -> CarEventReader: ...
    @staticmethod
    def write(file: BufferedWriter) -> None: ...
    @staticmethod
    def write_packed(file: BufferedWriter) -> None: ...

class CarState:
    class WheelSpeeds:
        fl: float
        fr: float
        rl: float
        rr: float
        @staticmethod
        @contextmanager
        def from_bytes(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> Iterator[CarState.WheelSpeedsReader]: ...
        @staticmethod
        def from_bytes_packed(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> CarState.WheelSpeedsReader: ...
        @staticmethod
        def new_message() -> CarState.WheelSpeedsBuilder: ...
        def to_dict(self) -> dict: ...

    class WheelSpeedsReader(CarState.WheelSpeeds):
        def as_builder(self) -> CarState.WheelSpeedsBuilder: ...

    class WheelSpeedsBuilder(CarState.WheelSpeeds):
        @staticmethod
        def from_dict(dictionary: dict) -> CarState.WheelSpeedsBuilder: ...
        def copy(self) -> CarState.WheelSpeedsBuilder: ...
        def to_bytes(self) -> bytes: ...
        def to_bytes_packed(self) -> bytes: ...
        def to_segments(self) -> list[bytes]: ...
        def as_reader(self) -> CarState.WheelSpeedsReader: ...
        @staticmethod
        def write(file: BufferedWriter) -> None: ...
        @staticmethod
        def write_packed(file: BufferedWriter) -> None: ...

    class CruiseState:
        enabled: bool
        speed: float
        available: bool
        speedOffset: float
        standstill: bool
        nonAdaptive: bool
        speedCluster: float
        @staticmethod
        @contextmanager
        def from_bytes(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> Iterator[CarState.CruiseStateReader]: ...
        @staticmethod
        def from_bytes_packed(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> CarState.CruiseStateReader: ...
        @staticmethod
        def new_message() -> CarState.CruiseStateBuilder: ...
        def to_dict(self) -> dict: ...

    class CruiseStateReader(CarState.CruiseState):
        def as_builder(self) -> CarState.CruiseStateBuilder: ...

    class CruiseStateBuilder(CarState.CruiseState):
        @staticmethod
        def from_dict(dictionary: dict) -> CarState.CruiseStateBuilder: ...
        def copy(self) -> CarState.CruiseStateBuilder: ...
        def to_bytes(self) -> bytes: ...
        def to_bytes_packed(self) -> bytes: ...
        def to_segments(self) -> list[bytes]: ...
        def as_reader(self) -> CarState.CruiseStateReader: ...
        @staticmethod
        def write(file: BufferedWriter) -> None: ...
        @staticmethod
        def write_packed(file: BufferedWriter) -> None: ...

    class ButtonEvent:
        Type = Literal[
            "unknown",
            "leftBlinker",
            "rightBlinker",
            "accelCruise",
            "decelCruise",
            "cancel",
            "altButton1",
            "altButton2",
            "altButton3",
            "setCruise",
            "resumeCruise",
            "gapAdjustCruise",
        ]
        pressed: bool
        type: CarState.ButtonEvent.Type
        @staticmethod
        @contextmanager
        def from_bytes(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> Iterator[CarState.ButtonEventReader]: ...
        @staticmethod
        def from_bytes_packed(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> CarState.ButtonEventReader: ...
        @staticmethod
        def new_message() -> CarState.ButtonEventBuilder: ...
        def to_dict(self) -> dict: ...

    class ButtonEventReader(CarState.ButtonEvent):
        def as_builder(self) -> CarState.ButtonEventBuilder: ...

    class ButtonEventBuilder(CarState.ButtonEvent):
        @staticmethod
        def from_dict(dictionary: dict) -> CarState.ButtonEventBuilder: ...
        def copy(self) -> CarState.ButtonEventBuilder: ...
        def to_bytes(self) -> bytes: ...
        def to_bytes_packed(self) -> bytes: ...
        def to_segments(self) -> list[bytes]: ...
        def as_reader(self) -> CarState.ButtonEventReader: ...
        @staticmethod
        def write(file: BufferedWriter) -> None: ...
        @staticmethod
        def write_packed(file: BufferedWriter) -> None: ...
    GearShifter = Literal["unknown", "park", "drive", "neutral", "reverse", "sport", "low", "brake", "eco", "manumatic"]
    errorsDEPRECATED: Sequence[CarEvent.EventName | CarEvent.EventNameBuilder | CarEvent.EventNameReader]
    vEgo: float
    wheelSpeeds: CarState.WheelSpeeds | CarState.WheelSpeedsBuilder | CarState.WheelSpeedsReader
    gas: float
    gasPressed: bool
    brake: float
    brakePressed: bool
    steeringAngleDeg: float
    steeringTorque: float
    steeringPressed: bool
    cruiseState: CarState.CruiseState | CarState.CruiseStateBuilder | CarState.CruiseStateReader
    buttonEvents: Sequence[CarState.ButtonEvent | CarState.ButtonEventBuilder | CarState.ButtonEventReader]
    canMonoTimesDEPRECATED: Sequence[int]
    events: Sequence[CarEvent | CarEventBuilder | CarEventReader]
    gearShifter: CarState.GearShifter
    steeringRateDeg: float
    aEgo: float
    vEgoRaw: float
    standstill: bool
    brakeLightsDEPRECATED: bool
    leftBlinker: bool
    rightBlinker: bool
    yawRate: float
    genericToggle: bool
    doorOpen: bool
    seatbeltUnlatched: bool
    canValid: bool
    steeringTorqueEps: float
    clutchPressed: bool
    steeringRateLimitedDEPRECATED: bool
    stockAeb: bool
    stockFcw: bool
    espDisabled: bool
    leftBlindspot: bool
    rightBlindspot: bool
    steerFaultTemporary: bool
    steerFaultPermanent: bool
    steeringAngleOffsetDeg: float
    brakeHoldActive: bool
    parkingBrake: bool
    canTimeout: bool
    fuelGauge: float
    accFaulted: bool
    charging: bool
    vEgoCluster: float
    regenBraking: bool
    engineRpm: float
    carFaultedNonCritical: bool
    @overload
    def init(self, name: Literal["wheelSpeeds"]) -> CarState.WheelSpeeds: ...
    @overload
    def init(self, name: Literal["cruiseState"]) -> CarState.CruiseState: ...
    @staticmethod
    @contextmanager
    def from_bytes(
        data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
    ) -> Iterator[CarStateReader]: ...
    @staticmethod
    def from_bytes_packed(
        data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
    ) -> CarStateReader: ...
    @staticmethod
    def new_message() -> CarStateBuilder: ...
    def to_dict(self) -> dict: ...

class CarStateReader(CarState):
    errorsDEPRECATED: Sequence[CarEvent.EventNameReader]
    wheelSpeeds: CarState.WheelSpeedsReader
    cruiseState: CarState.CruiseStateReader
    buttonEvents: Sequence[CarState.ButtonEventReader]
    events: Sequence[CarEventReader]
    def as_builder(self) -> CarStateBuilder: ...

class CarStateBuilder(CarState):
    errorsDEPRECATED: Sequence[CarEvent.EventName | CarEvent.EventNameBuilder | CarEvent.EventNameReader]
    wheelSpeeds: CarState.WheelSpeeds | CarState.WheelSpeedsBuilder | CarState.WheelSpeedsReader
    cruiseState: CarState.CruiseState | CarState.CruiseStateBuilder | CarState.CruiseStateReader
    buttonEvents: Sequence[CarState.ButtonEvent | CarState.ButtonEventBuilder | CarState.ButtonEventReader]
    events: Sequence[CarEvent | CarEventBuilder | CarEventReader]
    @staticmethod
    def from_dict(dictionary: dict) -> CarStateBuilder: ...
    def copy(self) -> CarStateBuilder: ...
    def to_bytes(self) -> bytes: ...
    def to_bytes_packed(self) -> bytes: ...
    def to_segments(self) -> list[bytes]: ...
    def as_reader(self) -> CarStateReader: ...
    @staticmethod
    def write(file: BufferedWriter) -> None: ...
    @staticmethod
    def write_packed(file: BufferedWriter) -> None: ...

class RadarData:
    Error = Literal["canError", "fault", "wrongConfig"]

    class RadarPoint:
        trackId: int
        dRel: float
        yRel: float
        vRel: float
        aRel: float
        yvRel: float
        measured: bool
        @staticmethod
        @contextmanager
        def from_bytes(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> Iterator[RadarData.RadarPointReader]: ...
        @staticmethod
        def from_bytes_packed(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> RadarData.RadarPointReader: ...
        @staticmethod
        def new_message() -> RadarData.RadarPointBuilder: ...
        def to_dict(self) -> dict: ...

    class RadarPointReader(RadarData.RadarPoint):
        def as_builder(self) -> RadarData.RadarPointBuilder: ...

    class RadarPointBuilder(RadarData.RadarPoint):
        @staticmethod
        def from_dict(dictionary: dict) -> RadarData.RadarPointBuilder: ...
        def copy(self) -> RadarData.RadarPointBuilder: ...
        def to_bytes(self) -> bytes: ...
        def to_bytes_packed(self) -> bytes: ...
        def to_segments(self) -> list[bytes]: ...
        def as_reader(self) -> RadarData.RadarPointReader: ...
        @staticmethod
        def write(file: BufferedWriter) -> None: ...
        @staticmethod
        def write_packed(file: BufferedWriter) -> None: ...
    errors: Sequence[RadarData.Error | RadarData.ErrorBuilder | RadarData.ErrorReader]
    points: Sequence[RadarData.RadarPoint | RadarData.RadarPointBuilder | RadarData.RadarPointReader]
    canMonoTimesDEPRECATED: Sequence[int]
    @staticmethod
    @contextmanager
    def from_bytes(
        data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
    ) -> Iterator[RadarDataReader]: ...
    @staticmethod
    def from_bytes_packed(
        data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
    ) -> RadarDataReader: ...
    @staticmethod
    def new_message() -> RadarDataBuilder: ...
    def to_dict(self) -> dict: ...

class RadarDataReader(RadarData):
    errors: Sequence[RadarData.ErrorReader]
    points: Sequence[RadarData.RadarPointReader]
    def as_builder(self) -> RadarDataBuilder: ...

class RadarDataBuilder(RadarData):
    errors: Sequence[RadarData.Error | RadarData.ErrorBuilder | RadarData.ErrorReader]
    points: Sequence[RadarData.RadarPoint | RadarData.RadarPointBuilder | RadarData.RadarPointReader]
    @staticmethod
    def from_dict(dictionary: dict) -> RadarDataBuilder: ...
    def copy(self) -> RadarDataBuilder: ...
    def to_bytes(self) -> bytes: ...
    def to_bytes_packed(self) -> bytes: ...
    def to_segments(self) -> list[bytes]: ...
    def as_reader(self) -> RadarDataReader: ...
    @staticmethod
    def write(file: BufferedWriter) -> None: ...
    @staticmethod
    def write_packed(file: BufferedWriter) -> None: ...

class CarControl:
    class CruiseControl:
        cancel: bool
        resume: bool
        speedOverrideDEPRECATED: float
        accelOverrideDEPRECATED: float
        override: bool
        @staticmethod
        @contextmanager
        def from_bytes(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> Iterator[CarControl.CruiseControlReader]: ...
        @staticmethod
        def from_bytes_packed(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> CarControl.CruiseControlReader: ...
        @staticmethod
        def new_message() -> CarControl.CruiseControlBuilder: ...
        def to_dict(self) -> dict: ...

    class CruiseControlReader(CarControl.CruiseControl):
        def as_builder(self) -> CarControl.CruiseControlBuilder: ...

    class CruiseControlBuilder(CarControl.CruiseControl):
        @staticmethod
        def from_dict(dictionary: dict) -> CarControl.CruiseControlBuilder: ...
        def copy(self) -> CarControl.CruiseControlBuilder: ...
        def to_bytes(self) -> bytes: ...
        def to_bytes_packed(self) -> bytes: ...
        def to_segments(self) -> list[bytes]: ...
        def as_reader(self) -> CarControl.CruiseControlReader: ...
        @staticmethod
        def write(file: BufferedWriter) -> None: ...
        @staticmethod
        def write_packed(file: BufferedWriter) -> None: ...

    class HUDControl:
        VisualAlert = Literal[
            "none", "fcw", "steerRequired", "brakePressed", "wrongGear", "seatbeltUnbuckled", "speedTooHigh", "ldw"
        ]
        AudibleAlert = Literal[
            "none",
            "engage",
            "disengage",
            "refuse",
            "warningSoft",
            "warningImmediate",
            "prompt",
            "promptRepeat",
            "promptDistracted",
        ]
        speedVisible: bool
        setSpeed: float
        lanesVisible: bool
        leadVisible: bool
        visualAlert: CarControl.HUDControl.VisualAlert
        audibleAlert: CarControl.HUDControl.AudibleAlert
        rightLaneVisible: bool
        leftLaneVisible: bool
        rightLaneDepart: bool
        leftLaneDepart: bool
        @staticmethod
        @contextmanager
        def from_bytes(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> Iterator[CarControl.HUDControlReader]: ...
        @staticmethod
        def from_bytes_packed(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> CarControl.HUDControlReader: ...
        @staticmethod
        def new_message() -> CarControl.HUDControlBuilder: ...
        def to_dict(self) -> dict: ...

    class HUDControlReader(CarControl.HUDControl):
        def as_builder(self) -> CarControl.HUDControlBuilder: ...

    class HUDControlBuilder(CarControl.HUDControl):
        @staticmethod
        def from_dict(dictionary: dict) -> CarControl.HUDControlBuilder: ...
        def copy(self) -> CarControl.HUDControlBuilder: ...
        def to_bytes(self) -> bytes: ...
        def to_bytes_packed(self) -> bytes: ...
        def to_segments(self) -> list[bytes]: ...
        def as_reader(self) -> CarControl.HUDControlReader: ...
        @staticmethod
        def write(file: BufferedWriter) -> None: ...
        @staticmethod
        def write_packed(file: BufferedWriter) -> None: ...

    class Actuators:
        LongControlState = Literal["off", "pid", "stopping", "starting"]
        gas: float
        brake: float
        steer: float
        steeringAngleDeg: float
        accel: float
        longControlState: CarControl.Actuators.LongControlState
        speed: float
        curvature: float
        steerOutputCan: float
        @staticmethod
        @contextmanager
        def from_bytes(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> Iterator[CarControl.ActuatorsReader]: ...
        @staticmethod
        def from_bytes_packed(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> CarControl.ActuatorsReader: ...
        @staticmethod
        def new_message() -> CarControl.ActuatorsBuilder: ...
        def to_dict(self) -> dict: ...

    class ActuatorsReader(CarControl.Actuators):
        def as_builder(self) -> CarControl.ActuatorsBuilder: ...

    class ActuatorsBuilder(CarControl.Actuators):
        @staticmethod
        def from_dict(dictionary: dict) -> CarControl.ActuatorsBuilder: ...
        def copy(self) -> CarControl.ActuatorsBuilder: ...
        def to_bytes(self) -> bytes: ...
        def to_bytes_packed(self) -> bytes: ...
        def to_segments(self) -> list[bytes]: ...
        def as_reader(self) -> CarControl.ActuatorsReader: ...
        @staticmethod
        def write(file: BufferedWriter) -> None: ...
        @staticmethod
        def write_packed(file: BufferedWriter) -> None: ...
    enabled: bool
    gasDEPRECATED: float
    brakeDEPRECATED: float
    steeringTorqueDEPRECATED: float
    cruiseControl: CarControl.CruiseControl | CarControl.CruiseControlBuilder | CarControl.CruiseControlReader
    hudControl: CarControl.HUDControl | CarControl.HUDControlBuilder | CarControl.HUDControlReader
    actuators: CarControl.Actuators | CarControl.ActuatorsBuilder | CarControl.ActuatorsReader
    activeDEPRECATED: bool
    rollDEPRECATED: float
    pitchDEPRECATED: float
    actuatorsOutput: CarControl.Actuators | CarControl.ActuatorsBuilder | CarControl.ActuatorsReader
    latActive: bool
    longActive: bool
    orientationNED: Sequence[float]
    angularVelocity: Sequence[float]
    leftBlinker: bool
    rightBlinker: bool
    @overload
    def init(self, name: Literal["cruiseControl"]) -> CarControl.CruiseControl: ...
    @overload
    def init(self, name: Literal["hudControl"]) -> CarControl.HUDControl: ...
    @overload
    def init(self, name: Literal["actuators"]) -> CarControl.Actuators: ...
    @overload
    def init(self, name: Literal["actuatorsOutput"]) -> CarControl.Actuators: ...
    @staticmethod
    @contextmanager
    def from_bytes(
        data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
    ) -> Iterator[CarControlReader]: ...
    @staticmethod
    def from_bytes_packed(
        data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
    ) -> CarControlReader: ...
    @staticmethod
    def new_message() -> CarControlBuilder: ...
    def to_dict(self) -> dict: ...

class CarControlReader(CarControl):
    cruiseControl: CarControl.CruiseControlReader
    hudControl: CarControl.HUDControlReader
    actuators: CarControl.ActuatorsReader
    actuatorsOutput: CarControl.ActuatorsReader
    def as_builder(self) -> CarControlBuilder: ...

class CarControlBuilder(CarControl):
    cruiseControl: CarControl.CruiseControl | CarControl.CruiseControlBuilder | CarControl.CruiseControlReader
    hudControl: CarControl.HUDControl | CarControl.HUDControlBuilder | CarControl.HUDControlReader
    actuators: CarControl.Actuators | CarControl.ActuatorsBuilder | CarControl.ActuatorsReader
    actuatorsOutput: CarControl.Actuators | CarControl.ActuatorsBuilder | CarControl.ActuatorsReader
    @staticmethod
    def from_dict(dictionary: dict) -> CarControlBuilder: ...
    def copy(self) -> CarControlBuilder: ...
    def to_bytes(self) -> bytes: ...
    def to_bytes_packed(self) -> bytes: ...
    def to_segments(self) -> list[bytes]: ...
    def as_reader(self) -> CarControlReader: ...
    @staticmethod
    def write(file: BufferedWriter) -> None: ...
    @staticmethod
    def write_packed(file: BufferedWriter) -> None: ...

class CarParams:
    SafetyModel = Literal[
        "silent",
        "hondaNidec",
        "toyota",
        "elm327",
        "gm",
        "hondaBoschGiraffe",
        "ford",
        "cadillac",
        "hyundai",
        "chrysler",
        "tesla",
        "subaru",
        "gmPassive",
        "mazda",
        "nissan",
        "volkswagen",
        "toyotaIpas",
        "allOutput",
        "gmAscm",
        "noOutput",
        "hondaBosch",
        "volkswagenPq",
        "subaruPreglobal",
        "hyundaiLegacy",
        "hyundaiCommunity",
        "volkswagenMlb",
        "hongqi",
        "body",
        "hyundaiCanfd",
        "volkswagenMqbEvo",
    ]

    class LongitudinalPIDTuning:
        kpBP: Sequence[float]
        kpV: Sequence[float]
        kiBP: Sequence[float]
        kiV: Sequence[float]
        deadzoneBP: Sequence[float]
        deadzoneV: Sequence[float]
        kf: float
        @staticmethod
        @contextmanager
        def from_bytes(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> Iterator[CarParams.LongitudinalPIDTuningReader]: ...
        @staticmethod
        def from_bytes_packed(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> CarParams.LongitudinalPIDTuningReader: ...
        @staticmethod
        def new_message() -> CarParams.LongitudinalPIDTuningBuilder: ...
        def to_dict(self) -> dict: ...

    class LongitudinalPIDTuningReader(CarParams.LongitudinalPIDTuning):
        def as_builder(self) -> CarParams.LongitudinalPIDTuningBuilder: ...

    class LongitudinalPIDTuningBuilder(CarParams.LongitudinalPIDTuning):
        @staticmethod
        def from_dict(dictionary: dict) -> CarParams.LongitudinalPIDTuningBuilder: ...
        def copy(self) -> CarParams.LongitudinalPIDTuningBuilder: ...
        def to_bytes(self) -> bytes: ...
        def to_bytes_packed(self) -> bytes: ...
        def to_segments(self) -> list[bytes]: ...
        def as_reader(self) -> CarParams.LongitudinalPIDTuningReader: ...
        @staticmethod
        def write(file: BufferedWriter) -> None: ...
        @staticmethod
        def write_packed(file: BufferedWriter) -> None: ...

    class LateralPIDTuning:
        kpBP: Sequence[float]
        kpV: Sequence[float]
        kiBP: Sequence[float]
        kiV: Sequence[float]
        kf: float
        @staticmethod
        @contextmanager
        def from_bytes(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> Iterator[CarParams.LateralPIDTuningReader]: ...
        @staticmethod
        def from_bytes_packed(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> CarParams.LateralPIDTuningReader: ...
        @staticmethod
        def new_message() -> CarParams.LateralPIDTuningBuilder: ...
        def to_dict(self) -> dict: ...

    class LateralPIDTuningReader(CarParams.LateralPIDTuning):
        def as_builder(self) -> CarParams.LateralPIDTuningBuilder: ...

    class LateralPIDTuningBuilder(CarParams.LateralPIDTuning):
        @staticmethod
        def from_dict(dictionary: dict) -> CarParams.LateralPIDTuningBuilder: ...
        def copy(self) -> CarParams.LateralPIDTuningBuilder: ...
        def to_bytes(self) -> bytes: ...
        def to_bytes_packed(self) -> bytes: ...
        def to_segments(self) -> list[bytes]: ...
        def as_reader(self) -> CarParams.LateralPIDTuningReader: ...
        @staticmethod
        def write(file: BufferedWriter) -> None: ...
        @staticmethod
        def write_packed(file: BufferedWriter) -> None: ...

    class LateralINDITuning:
        outerLoopGainDEPRECATED: float
        innerLoopGainDEPRECATED: float
        timeConstantDEPRECATED: float
        actuatorEffectivenessDEPRECATED: float
        outerLoopGainBP: Sequence[float]
        outerLoopGainV: Sequence[float]
        innerLoopGainBP: Sequence[float]
        innerLoopGainV: Sequence[float]
        timeConstantBP: Sequence[float]
        timeConstantV: Sequence[float]
        actuatorEffectivenessBP: Sequence[float]
        actuatorEffectivenessV: Sequence[float]
        @staticmethod
        @contextmanager
        def from_bytes(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> Iterator[CarParams.LateralINDITuningReader]: ...
        @staticmethod
        def from_bytes_packed(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> CarParams.LateralINDITuningReader: ...
        @staticmethod
        def new_message() -> CarParams.LateralINDITuningBuilder: ...
        def to_dict(self) -> dict: ...

    class LateralINDITuningReader(CarParams.LateralINDITuning):
        def as_builder(self) -> CarParams.LateralINDITuningBuilder: ...

    class LateralINDITuningBuilder(CarParams.LateralINDITuning):
        @staticmethod
        def from_dict(dictionary: dict) -> CarParams.LateralINDITuningBuilder: ...
        def copy(self) -> CarParams.LateralINDITuningBuilder: ...
        def to_bytes(self) -> bytes: ...
        def to_bytes_packed(self) -> bytes: ...
        def to_segments(self) -> list[bytes]: ...
        def as_reader(self) -> CarParams.LateralINDITuningReader: ...
        @staticmethod
        def write(file: BufferedWriter) -> None: ...
        @staticmethod
        def write_packed(file: BufferedWriter) -> None: ...

    class LateralLQRTuning:
        scale: float
        ki: float
        dcGain: float
        a: Sequence[float]
        b: Sequence[float]
        c: Sequence[float]
        k: Sequence[float]
        l: Sequence[float]
        @staticmethod
        @contextmanager
        def from_bytes(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> Iterator[CarParams.LateralLQRTuningReader]: ...
        @staticmethod
        def from_bytes_packed(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> CarParams.LateralLQRTuningReader: ...
        @staticmethod
        def new_message() -> CarParams.LateralLQRTuningBuilder: ...
        def to_dict(self) -> dict: ...

    class LateralLQRTuningReader(CarParams.LateralLQRTuning):
        def as_builder(self) -> CarParams.LateralLQRTuningBuilder: ...

    class LateralLQRTuningBuilder(CarParams.LateralLQRTuning):
        @staticmethod
        def from_dict(dictionary: dict) -> CarParams.LateralLQRTuningBuilder: ...
        def copy(self) -> CarParams.LateralLQRTuningBuilder: ...
        def to_bytes(self) -> bytes: ...
        def to_bytes_packed(self) -> bytes: ...
        def to_segments(self) -> list[bytes]: ...
        def as_reader(self) -> CarParams.LateralLQRTuningReader: ...
        @staticmethod
        def write(file: BufferedWriter) -> None: ...
        @staticmethod
        def write_packed(file: BufferedWriter) -> None: ...

    class LateralTorqueTuning:
        useSteeringAngle: bool
        kp: float
        ki: float
        friction: float
        kf: float
        steeringAngleDeadzoneDeg: float
        latAccelFactor: float
        latAccelOffset: float
        @staticmethod
        @contextmanager
        def from_bytes(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> Iterator[CarParams.LateralTorqueTuningReader]: ...
        @staticmethod
        def from_bytes_packed(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> CarParams.LateralTorqueTuningReader: ...
        @staticmethod
        def new_message() -> CarParams.LateralTorqueTuningBuilder: ...
        def to_dict(self) -> dict: ...

    class LateralTorqueTuningReader(CarParams.LateralTorqueTuning):
        def as_builder(self) -> CarParams.LateralTorqueTuningBuilder: ...

    class LateralTorqueTuningBuilder(CarParams.LateralTorqueTuning):
        @staticmethod
        def from_dict(dictionary: dict) -> CarParams.LateralTorqueTuningBuilder: ...
        def copy(self) -> CarParams.LateralTorqueTuningBuilder: ...
        def to_bytes(self) -> bytes: ...
        def to_bytes_packed(self) -> bytes: ...
        def to_segments(self) -> list[bytes]: ...
        def as_reader(self) -> CarParams.LateralTorqueTuningReader: ...
        @staticmethod
        def write(file: BufferedWriter) -> None: ...
        @staticmethod
        def write_packed(file: BufferedWriter) -> None: ...

    class LateralTuning:
        pid: CarParams.LateralPIDTuning | CarParams.LateralPIDTuningBuilder | CarParams.LateralPIDTuningReader
        indi: CarParams.LateralINDITuning | CarParams.LateralINDITuningBuilder | CarParams.LateralINDITuningReader
        lqr: CarParams.LateralLQRTuning | CarParams.LateralLQRTuningBuilder | CarParams.LateralLQRTuningReader
        torque: CarParams.LateralTorqueTuning | CarParams.LateralTorqueTuningBuilder | CarParams.LateralTorqueTuningReader
        def which(self) -> Literal["pid", "indi", "lqr", "torque"]: ...
        @overload
        def init(self, name: Literal["pid"]) -> CarParams.LateralPIDTuning: ...
        @overload
        def init(self, name: Literal["indi"]) -> CarParams.LateralINDITuning: ...
        @overload
        def init(self, name: Literal["lqr"]) -> CarParams.LateralLQRTuning: ...
        @overload
        def init(self, name: Literal["torque"]) -> CarParams.LateralTorqueTuning: ...
        @staticmethod
        @contextmanager
        def from_bytes(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> Iterator[CarParams.LateralTuningReader]: ...
        @staticmethod
        def from_bytes_packed(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> CarParams.LateralTuningReader: ...
        @staticmethod
        def new_message() -> CarParams.LateralTuningBuilder: ...
        def to_dict(self) -> dict: ...

    class LateralTuningReader(CarParams.LateralTuning):
        pid: CarParams.LateralPIDTuningReader
        indi: CarParams.LateralINDITuningReader
        lqr: CarParams.LateralLQRTuningReader
        torque: CarParams.LateralTorqueTuningReader
        def as_builder(self) -> CarParams.LateralTuningBuilder: ...

    class LateralTuningBuilder(CarParams.LateralTuning):
        pid: CarParams.LateralPIDTuning | CarParams.LateralPIDTuningBuilder | CarParams.LateralPIDTuningReader
        indi: CarParams.LateralINDITuning | CarParams.LateralINDITuningBuilder | CarParams.LateralINDITuningReader
        lqr: CarParams.LateralLQRTuning | CarParams.LateralLQRTuningBuilder | CarParams.LateralLQRTuningReader
        torque: CarParams.LateralTorqueTuning | CarParams.LateralTorqueTuningBuilder | CarParams.LateralTorqueTuningReader
        @staticmethod
        def from_dict(dictionary: dict) -> CarParams.LateralTuningBuilder: ...
        def copy(self) -> CarParams.LateralTuningBuilder: ...
        def to_bytes(self) -> bytes: ...
        def to_bytes_packed(self) -> bytes: ...
        def to_segments(self) -> list[bytes]: ...
        def as_reader(self) -> CarParams.LateralTuningReader: ...
        @staticmethod
        def write(file: BufferedWriter) -> None: ...
        @staticmethod
        def write_packed(file: BufferedWriter) -> None: ...
    SteerControlType = Literal["torque", "angle", "curvature"]
    TransmissionType = Literal["unknown", "automatic", "manual", "direct", "cvt"]

    class CarFw:
        Ecu = Literal[
            "eps",
            "abs",
            "fwdRadar",
            "fwdCamera",
            "engine",
            "unknown",
            "dsu",
            "parkingAdas",
            "transmission",
            "srs",
            "gateway",
            "hud",
            "combinationMeter",
            "vsa",
            "programmedFuelInjection",
            "electricBrakeBooster",
            "shiftByWire",
            "debug",
            "hybrid",
            "adas",
            "hvac",
            "cornerRadar",
            "epb",
            "telematics",
            "body",
        ]
        ecu: CarParams.CarFw.Ecu
        fwVersion: bytes
        address: int
        subAddress: int
        responseAddress: int
        request: Sequence[bytes]
        brand: str
        bus: int
        logging: bool
        obdMultiplexing: bool
        @staticmethod
        @contextmanager
        def from_bytes(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> Iterator[CarParams.CarFwReader]: ...
        @staticmethod
        def from_bytes_packed(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> CarParams.CarFwReader: ...
        @staticmethod
        def new_message() -> CarParams.CarFwBuilder: ...
        def to_dict(self) -> dict: ...

    class CarFwReader(CarParams.CarFw):
        def as_builder(self) -> CarParams.CarFwBuilder: ...

    class CarFwBuilder(CarParams.CarFw):
        @staticmethod
        def from_dict(dictionary: dict) -> CarParams.CarFwBuilder: ...
        def copy(self) -> CarParams.CarFwBuilder: ...
        def to_bytes(self) -> bytes: ...
        def to_bytes_packed(self) -> bytes: ...
        def to_segments(self) -> list[bytes]: ...
        def as_reader(self) -> CarParams.CarFwReader: ...
        @staticmethod
        def write(file: BufferedWriter) -> None: ...
        @staticmethod
        def write_packed(file: BufferedWriter) -> None: ...

    class LateralParams:
        torqueBP: Sequence[int]
        torqueV: Sequence[int]
        @staticmethod
        @contextmanager
        def from_bytes(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> Iterator[CarParams.LateralParamsReader]: ...
        @staticmethod
        def from_bytes_packed(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> CarParams.LateralParamsReader: ...
        @staticmethod
        def new_message() -> CarParams.LateralParamsBuilder: ...
        def to_dict(self) -> dict: ...

    class LateralParamsReader(CarParams.LateralParams):
        def as_builder(self) -> CarParams.LateralParamsBuilder: ...

    class LateralParamsBuilder(CarParams.LateralParams):
        @staticmethod
        def from_dict(dictionary: dict) -> CarParams.LateralParamsBuilder: ...
        def copy(self) -> CarParams.LateralParamsBuilder: ...
        def to_bytes(self) -> bytes: ...
        def to_bytes_packed(self) -> bytes: ...
        def to_segments(self) -> list[bytes]: ...
        def as_reader(self) -> CarParams.LateralParamsReader: ...
        @staticmethod
        def write(file: BufferedWriter) -> None: ...
        @staticmethod
        def write_packed(file: BufferedWriter) -> None: ...
    FingerprintSource = Literal["can", "fw", "fixed"]
    NetworkLocation = Literal["fwdCamera", "gateway"]

    class SafetyConfig:
        safetyModel: CarParams.SafetyModel
        safetyParamDEPRECATED: int
        safetyParam2DEPRECATED: int
        safetyParam: int
        @staticmethod
        @contextmanager
        def from_bytes(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> Iterator[CarParams.SafetyConfigReader]: ...
        @staticmethod
        def from_bytes_packed(
            data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
        ) -> CarParams.SafetyConfigReader: ...
        @staticmethod
        def new_message() -> CarParams.SafetyConfigBuilder: ...
        def to_dict(self) -> dict: ...

    class SafetyConfigReader(CarParams.SafetyConfig):
        def as_builder(self) -> CarParams.SafetyConfigBuilder: ...

    class SafetyConfigBuilder(CarParams.SafetyConfig):
        @staticmethod
        def from_dict(dictionary: dict) -> CarParams.SafetyConfigBuilder: ...
        def copy(self) -> CarParams.SafetyConfigBuilder: ...
        def to_bytes(self) -> bytes: ...
        def to_bytes_packed(self) -> bytes: ...
        def to_segments(self) -> list[bytes]: ...
        def as_reader(self) -> CarParams.SafetyConfigReader: ...
        @staticmethod
        def write(file: BufferedWriter) -> None: ...
        @staticmethod
        def write_packed(file: BufferedWriter) -> None: ...
    carName: str
    carFingerprint: str
    enableGasInterceptor: bool
    pcmCruise: bool
    enableCameraDEPRECATED: bool
    enableDsu: bool
    enableApgsDEPRECATED: bool
    minEnableSpeed: float
    minSteerSpeed: float
    safetyModelDEPRECATED: CarParams.SafetyModel
    safetyParamDEPRECATED: int
    steerMaxBPDEPRECATED: Sequence[float]
    steerMaxVDEPRECATED: Sequence[float]
    gasMaxBPDEPRECATED: Sequence[float]
    gasMaxVDEPRECATED: Sequence[float]
    brakeMaxBPDEPRECATED: Sequence[float]
    brakeMaxVDEPRECATED: Sequence[float]
    mass: float
    wheelbase: float
    centerToFront: float
    steerRatio: float
    steerRatioRear: float
    rotationalInertia: float
    tireStiffnessFront: float
    tireStiffnessRear: float
    longitudinalTuning: CarParams.LongitudinalPIDTuning | CarParams.LongitudinalPIDTuningBuilder | CarParams.LongitudinalPIDTuningReader
    lateralTuning: CarParams.LateralTuning | CarParams.LateralTuningBuilder | CarParams.LateralTuningReader
    steerLimitAlert: bool
    vEgoStopping: float
    directAccelControlDEPRECATED: bool
    stoppingControl: bool
    startAccel: float
    steerRateCostDEPRECATED: float
    steerControlType: CarParams.SteerControlType
    radarUnavailable: bool
    steerActuatorDelay: float
    openpilotLongitudinalControl: bool
    carVin: str
    isPandaBlackDEPRECATED: bool
    dashcamOnly: bool
    safetyModelPassiveDEPRECATED: CarParams.SafetyModel
    transmissionType: CarParams.TransmissionType
    carFw: Sequence[CarParams.CarFw | CarParams.CarFwBuilder | CarParams.CarFwReader]
    radarTimeStep: float
    communityFeatureDEPRECATED: bool
    steerLimitTimer: float
    lateralParams: CarParams.LateralParams | CarParams.LateralParamsBuilder | CarParams.LateralParamsReader
    fingerprintSource: CarParams.FingerprintSource
    networkLocation: CarParams.NetworkLocation
    minSpeedCanDEPRECATED: float
    stoppingDecelRate: float
    startingAccelRateDEPRECATED: float
    maxSteeringAngleDegDEPRECATED: float
    fuzzyFingerprint: bool
    enableBsm: bool
    hasStockCameraDEPRECATED: bool
    longitudinalActuatorDelayUpperBound: float
    vEgoStarting: float
    stopAccel: float
    longitudinalActuatorDelayLowerBound: float
    safetyConfigs: Sequence[CarParams.SafetyConfig | CarParams.SafetyConfigBuilder | CarParams.SafetyConfigReader]
    wheelSpeedFactor: float
    flags: int
    alternativeExperience: int
    notCar: bool
    maxLateralAccel: float
    autoResumeSng: bool
    startingState: bool
    experimentalLongitudinalAvailable: bool
    tireStiffnessFactor: float
    @overload
    def init(self, name: Literal["longitudinalTuning"]) -> CarParams.LongitudinalPIDTuning: ...
    @overload
    def init(self, name: Literal["lateralTuning"]) -> LateralTuning: ...
    @overload
    def init(self, name: Literal["lateralParams"]) -> CarParams.LateralParams: ...
    @staticmethod
    @contextmanager
    def from_bytes(
        data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
    ) -> Iterator[CarParamsReader]: ...
    @staticmethod
    def from_bytes_packed(
        data: bytes, traversal_limit_in_words: int | None = ..., nesting_limit: int | None = ...
    ) -> CarParamsReader: ...
    @staticmethod
    def new_message() -> CarParamsBuilder: ...
    def to_dict(self) -> dict: ...

class CarParamsReader(CarParams):
    longitudinalTuning: CarParams.LongitudinalPIDTuningReader
    lateralTuning: CarParams.LateralTuningReader
    carFw: Sequence[CarParams.CarFwReader]
    lateralParams: CarParams.LateralParamsReader
    safetyConfigs: Sequence[CarParams.SafetyConfigReader]
    def as_builder(self) -> CarParamsBuilder: ...

class CarParamsBuilder(CarParams):
    longitudinalTuning: CarParams.LongitudinalPIDTuning | CarParams.LongitudinalPIDTuningBuilder | CarParams.LongitudinalPIDTuningReader
    lateralTuning: CarParams.LateralTuning | CarParams.LateralTuningBuilder | CarParams.LateralTuningReader
    carFw: Sequence[CarParams.CarFw | CarParams.CarFwBuilder | CarParams.CarFwReader]
    lateralParams: CarParams.LateralParams | CarParams.LateralParamsBuilder | CarParams.LateralParamsReader
    safetyConfigs: Sequence[CarParams.SafetyConfig | CarParams.SafetyConfigBuilder | CarParams.SafetyConfigReader]
    @staticmethod
    def from_dict(dictionary: dict) -> CarParamsBuilder: ...
    def copy(self) -> CarParamsBuilder: ...
    def to_bytes(self) -> bytes: ...
    def to_bytes_packed(self) -> bytes: ...
    def to_segments(self) -> list[bytes]: ...
    def as_reader(self) -> CarParamsReader: ...
    @staticmethod
    def write(file: BufferedWriter) -> None: ...
    @staticmethod
    def write_packed(file: BufferedWriter) -> None: ...
