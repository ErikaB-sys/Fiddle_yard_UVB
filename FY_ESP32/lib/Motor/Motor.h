#pragma once
#include "FY_Bringup.h"

namespace fy {
// ESP32-only commissioning extension. Nano command IDs in Protokoll.h
// remain untouched: legacy LEFT=0x0B and RIGHT=0x0D are NOT MOVE.
constexpr uint8_t CMD_MOVE = 0x0FU;

// Maximum displacement PER command without a valid reference.
// BENCH ONLY: no cumulative limit, end-stop protection or FY mechanics.
constexpr uint16_t FY_MOVE_MAX_UNREFERENCED_STEPS = 10000U;

// Byte 0: high nibble = microstep mode, bit 0 = direction,
//         bits 3..1 reserved and must be zero.
// Byte 1: step count low byte; byte 2: step count high byte.
enum class MoveMicrostep : uint8_t {
    FULL = 0U, HALF = 1U, QUARTER = 2U, EIGHTH = 3U, SIXTEENTH = 4U
};

// Fixed hardware resolution: MS pins are configured once at motor startup.
constexpr MoveMicrostep FY_MOTOR_MICROSTEP_MODE = MoveMicrostep::EIGHTH;

struct MoveRequest {
    bool directionRight = false; // 0=LEFT, 1=RIGHT
    MoveMicrostep mode = FY_MOTOR_MICROSTEP_MODE;
    uint16_t steps = 0U;
};

// Shared payload decoder; implementation stays in Motor.cpp.
bool decodeMovePayload(const uint8_t* data, MoveRequest& result);

// Ported verbatim in meaning from Nano Motor.h: motor-owned job/payload.
struct MotorJob_t {
    bool valid;
    uint8_t cmd;
    uint8_t data[MAX_COMMAND_LENGTH - 1];
};

// Nano motor-state and six-phase profile model; REF remains invalid until sensors are integrated.
enum class MotorState_t { IDLE, CONF, REFRUN, MOVING, STOPPED, ERROR };
enum class MotorProfile_t { ACC1, ACC2, KONST, BRE1, BRE2, POSI };
struct Profilelement_t { uint16_t Steps; int16_t Accel; };
struct MotorProfileParam_t {
    uint16_t PosiMin;
    uint16_t KonstMin;
    uint16_t Acc1Steps;
    uint16_t Acc2Steps;
    uint16_t Bre1Steps;
    uint16_t Bre2Steps;
    uint16_t VMin;
    uint16_t VMax;
    uint16_t VPos;
    uint16_t Acc1Accel;
    uint16_t Acc2Accel;
    uint16_t Bre1Accel;
    uint16_t Bre2Accel;
    uint16_t Acc1RampInterval;
    uint16_t Acc2RampInterval;
    uint16_t Bre1RampInterval;
    uint16_t Bre2RampInterval;
};
enum class MotorCommandResult_t : uint8_t { ACCEPTED, INVALID_DATA, STOP_ACTIVE, BUSY };
enum class MotorJobResult_t { NONE, DONE, ENDSTOP_LEFT, ENDSTOP_RIGHT };

// Ported six-phase Nano motor profile calculation, no timer dependency.
// Other Motor state and ISR functions will be integrated in the next step.
class Motor {
public:
    /** @brief Berechnet die sechs Profilabschnitte fuer eine Fahrstrecke. */
    bool calcProfile(uint16_t distance);
    // Parameter preparation for unreferenced commissioning moves.
    // Motor operational state (GO, STOP, MOVING) is still owned by Muninn.
    /** @brief Prueft und bereitet einen relativen MOVE-Auftrag vor. */
    MotorCommandResult_t prepareMove(const uint8_t* data,
                                     MoveRequest& request);
    // Staged Motor state machine; ownership remains solely on Muninn/core 1.
    MotorState_t state() const { return _State; }
    bool referenced() const { return _ReferenceValid; }
    /** @brief Wechselt nach GO-Freigabe gegebenenfalls in IDLE. */
    void onGoEnabled();
    /** @brief Uebernimmt den STOP-Zustand und verwirft die aktuelle Referenz. */
    void onStop(bool latched);
    /** @brief Kennzeichnet den Beginn einer Bewegung. */
    void onMoveStarted();
    /** @brief Kennzeichnet den Abschluss einer Bewegung. */
    void onMoveFinished();
    /** @brief Setzt den Motorzustand nach einem Profilfehler auf ERROR. */
    void onProfileError();
    /** @brief Kennzeichnet den Beginn einer Referenzfahrt. */
    void onReferenceStarted();
    /** @brief Uebernimmt das Ergebnis einer Referenzfahrt. */
    void onReferenceFinished(bool success);
    /** @brief Prueft einen Referenzauftrag. */
    MotorCommandResult_t prepareReference(uint8_t mode);
    // Nano HW_TEST reference_run() port: state and calibration belong to Motor.
    // StepTimer is the only ESP32-specific sensor/STEP bridge.
    /** @brief Startet die physische Referenzsequenz. */
    bool referenceBegin(bool full);
    /** @brief Bricht eine laufende Referenzsequenz ab. */
    void referenceCancel();
    /** @brief Verarbeitet den naechsten Referenz-Timerabschluss. */
    void referenceUpdate();
    bool referenceCalibrated() const { return _calibrated; }
    bool referenceActive() const { return _active; }
    bool referenceSuccessful() const { return _stage == RefRunState::DONE; }
    uint32_t referencePosition() const { return _position; }
    uint32_t referenceOppositeEnd() const { return _oppositeEnd; }
    // Boot-Load, Defaults und physikalische Langreferenz-Kalibrierung.
    /** @brief Laedt persistente Kalibrierung, Gleise und Rampenparameter. */
    void initializeStorage();
    bool calibrationSavePending() const { return _calibrationPending; }
    /** @brief Speichert eine vorgemerkte Kalibrierung. */
    bool persistCalibration();
    /** @brief Liest eine gespeicherte Gleisposition. */
    bool trackPosition(uint8_t track, uint32_t& out) const;
    /** @brief Setzt alle Gleis-Defaults erst im NVM und danach im RAM. */
    bool restoreTrackDefaults();
    /** @brief Liest einen Rampenparameter. */
    bool getRampParameter(uint8_t id, uint32_t& value) const;
    /** @brief Aendert einen Rampenparameter im RAM. */
    bool setRampParameter(uint8_t id, uint32_t value);
    /** @brief Speichert die Rampenparameter. */
    bool saveRampParameters(bool& changed);
    /** @brief Stellt die Rampen-Standardwerte wieder her. */
    bool restoreRampDefaults(bool& changed);
    bool rampParametersValid() const { return _rampParametersValid; }
    void invalidateCurrentReference() { _ReferenceValid = false; }
    /** @brief Aendert eine Gleisposition nur im RAM; prueft den Wertebereich. */
    /** @brief Aendert eine Gleisposition im RAM. */
    bool setTrackPosition(uint8_t track, uint32_t position);
    /** @brief Speichert die aktuellen Gleispositionen persistent mit CRC. */
    /** @brief Speichert die Gleispositionen persistent. */
    bool saveTrackPositions();
    bool tracksValid() const { return _tracksValid; }
    bool hasNvmFault() const { return _nvmFaultMask != 0U; }

    // Synthetic short REF (without motion) is BENCH ONLY, never sets _ReferenceValid.
    /** @brief Setzt die simulierte Testposition zurueck. */
    void resetBenchPosition();  // Called exclusively after accepted fake REFERENCE 00.
    /** @brief Verwirft die simulierte Testposition. */
    void invalidateBenchPosition();
    bool benchPositionValid() const { return _BenchPositionValid; }
    bool benchReferenceValid() const { return _BenchReferenceValid && _BenchPositionValid; }
    uint16_t benchPosition() const { return _BenchPosition; }
    /** @brief Bereitet eine Fahrt zu einem Gleis vor. */
    MotorCommandResult_t prepareSetTrack(uint8_t track, MoveRequest& request);
    /** @brief Bereitet eine Fahrt zu einer absoluten Position vor. */
    MotorCommandResult_t prepareSetPosition(uint16_t target,
                                             MoveRequest& request);
    /** @brief Uebernimmt eine angenommene Bewegung. */
    void onMotionAccepted(const MoveRequest& request);
    /** @brief Aktualisiert den Fortschritt einer Bewegung. */
    void onMotionProgress(uint32_t completedSteps);
    /** @brief Prueft die Gueltigkeit des Fahrprofils. */
    bool isProfileValid(uint16_t distance) const;
    const Profilelement_t* profile() const { return Motor_profil; }
    const MotorProfileParam_t& profileParams() const { return _ProfileParam; }
private:

    enum class RefRunState : uint8_t {
        IDLE, SEEK_MOTOR, SEEK_OPPOSITE, APPROACH, FIRST_PASS,
        BACKOFF, SECOND_PASS, DONE, ERROR
    };
    /** @brief Startet einen Referenzabschnitt ueber StepTimer. */
    bool referenceLaunch(bool away, uint16_t hz, bool stopAtExit,
                         bool coarse, uint16_t steps = UINT16_MAX);
    /** @brief Schliesst eine Referenzsequenz erfolgreich ab. */
    void referenceFinish();
    /** @brief Beendet die Referenzsequenz mit Fehler. */
    void referenceFail();
    RefRunState _stage = RefRunState::IDLE;
    bool _active = false;
    bool _full = false;
    bool _calibrated = false;
    bool _calibrationPending = false;
    bool _storedCalibValid = false;
    bool _tracksValid = false;
    bool _rampParametersValid = false;
    uint8_t _nvmFaultMask = 0U;
    uint32_t _savedCalibration[3]{};
    uint32_t _measuredCalibration[3]{};
    uint32_t _tracks[5]{};
    uint32_t _position = 0U;
    uint32_t _coarse = 0U;
    uint32_t _oppositeEnd = 0U;
    uint32_t _refLeft = 0U;
    uint32_t _refRight = 0U;
    uint32_t _refLeftReverse = 0U;
    uint32_t _refRightReverse = 0U;
    MotorState_t _State = MotorState_t::STOPPED; // POR/STOP latch.
    bool _ReferenceValid = false; // Real LS reference remains invalid.
    bool _BenchPositionValid = false;
    bool _BenchReferenceValid = false;
    bool _MotionPositionTracked = false;
    uint16_t _BenchPosition = 0U;
    uint16_t _MotionStartPosition = 0U;
    uint16_t _MotionStepCount = 0U;
    bool _MotionRight = false;
    bool _StopActive = true;
    bool _TimerValid = false;
    // Effective intervals of Nano current_profile_ramp_interval(): 1,2,2,1.
    MotorProfileParam_t _ProfileParam{160,400,100,100,100,100,120,800,120,2,1,1,2,1,2,2,1};
    Profilelement_t Motor_profil[6]{{100,-2},{100,-1},{200,0},{100,+1},{100,+2},{10,0}};
};

// Motor state-machine skeleton is active for MOVE/GO/STOP; complete Nano
// reference flow remains pending. The six-phase timer is coded but not yet HW-tested.
// Thread-safe driver-enable snapshot, published by Muninn (core 1).
// GPIO25/A4988 ENABLE is active LOW (HW3). Not a movement status.
bool Muninn_isDriverEnabled();
// Atomic movement snapshot; true while the bounded STEP train is running.
bool Muninn_isMoving();
// 0=Stillstand, 1=physisch LINKS/G39, 2=physisch RECHTS/G36.
// Liest die reale DIR-Ausgabe nur bei aktiver Bewegung, ohne ISR-Zugriff.
uint8_t Muninn_motionDirectionSnapshot();
// A simulated quarter-step position only in FY_MOTOR_BENCH_TEST.
// Otherwise returns Nano POSITION_UNKNOWN (0x00007FFF).
uint32_t Muninn_positionSnapshot();
// Explicit reference truth source: 0=UNKNOWN, 1=REAL, 2=BENCH_FAKE.
uint8_t Muninn_referenceStatusSnapshot();
// Packed STATUS_Track payload: target | (last confirmed << 8) | (status << 16).
uint32_t Muninn_trackSnapshot();
// ID-spezifischer atomarer Parameterwert. False bei ungueltigem/fehlendem Wert.
bool Muninn_getParamSnapshot(uint8_t id, uint32_t& value);
// NVM-Fehlermaske / Gueltigkeitsmaske in einem atomaren uint16-Snapshot.
uint16_t Muninn_nvmStatusSnapshot();
// Atomic status snapshot: IDLE=0, CONF=1, REFRUN=2, MOVING=3, STOPPED=4, ERROR=5.
MotorState_t Muninn_motorState();

void Muninn_initMotorMock();
BaseType_t Muninn_startMotorMock(BringupContext* context);
}
