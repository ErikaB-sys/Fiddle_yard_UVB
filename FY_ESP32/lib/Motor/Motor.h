#pragma once
#include "FY_Bringup.h"

namespace fy {
// ESP32-Erweiterung fuer die Inbetriebnahme. Nano-Befehlskennungen bleiben
// unveraendert: LEFT=0x0B und RIGHT=0x0D sind nicht MOVE.
constexpr uint8_t CMD_MOVE = 0x0FU;

// Maximale Schrittzahl je Befehl ohne gueltige Referenz.
// Nur Testbetrieb: keine kumulative Grenze und keine mechanische Wegfreigabe.
constexpr uint16_t FY_MOVE_MAX_UNREFERENCED_STEPS = 10000U;

// Byte 0: oberes Halbbyte = Mikroschrittmodus, Bit 0 = Richtung;
//         Bits 3..1 sind reserviert und muessen null sein.
// Byte 1: Schrittzahl Low-Byte; Byte 2: High-Byte.
enum class MoveMicrostep : uint8_t {
    FULL = 0U, HALF = 1U, QUARTER = 2U, EIGHTH = 3U, SIXTEENTH = 4U
};

// Feste Hardware-Aufloesung: MS-Pins werden beim Motorstart gesetzt.
constexpr MoveMicrostep FY_MOTOR_MICROSTEP_MODE = MoveMicrostep::EIGHTH;

struct MoveRequest {
    bool directionRight = false; // 0=LINKS, 1=RECHTS
    MoveMicrostep mode = FY_MOTOR_MICROSTEP_MODE;
    uint16_t steps = 0U;
};

// Gemeinsamer Nutzdaten-Decoder; Implementierung in Motor.cpp.
bool decodeMovePayload(const uint8_t* data, MoveRequest& result);

// Inhaltlich vom Nano uebernommener Motorauftrag mit Nutzdaten.
struct MotorJob_t {
    bool valid;
    uint8_t cmd;
    uint8_t data[MAX_COMMAND_LENGTH - 1];
};

// Nano-Motorzustaende und sechsstufiges Fahrprofil; reale REF aus Sensorik.
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

// Sechsstufige Nano-Profilberechnung ohne direkte Timerabhaengigkeit.
class Motor {
public:
    /** @brief Berechnet die sechs Profilabschnitte fuer eine Fahrstrecke. */
    bool calcProfile(uint16_t distance);
    // Vorbereitung nicht referenzierter Testfahrten.
    // Muninn besitzt den Betriebszustand (GO, STOP, MOVING).
    /** @brief Prueft und bereitet einen relativen MOVE-Auftrag vor. */
    MotorCommandResult_t prepareMove(const uint8_t* data,
                                     MoveRequest& request);
    // Motorzustandsautomat; alleiniger Eigentuemer ist Muninn/Core 1.
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
    // Portierung des Nano-Referenzlaufs: Zustand und Kalibrierung liegen im Motor.
    // StepTimer verbindet Sensorik und STEP-Hardware.
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
        bool setTrackPosition(uint8_t track, uint32_t position);
    /** @brief Speichert die aktuellen Gleispositionen persistent mit CRC. */
        bool saveTrackPositions();
    bool tracksValid() const { return _tracksValid; }
    bool hasNvmFault() const { return _nvmFaultMask != 0U; }

    // Simulierte Kurzreferenz ohne Fahrt nur im Testbetrieb; setzt nie _ReferenceValid.
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
    // Wirksame Nano-Rampenintervalle: 1, 2, 2, 1.
    MotorProfileParam_t _ProfileParam{160,400,100,100,100,100,120,800,120,2,1,1,2,1,2,2,1};
    Profilelement_t Motor_profil[6]{{100,-2},{100,-1},{200,0},{100,+1},{100,+2},{10,0}};
};

// Atomarer Treiberfreigabe-Snapshot von Muninn/Core 1.
// GPIO25/A4988 ENABLE ist LOW-aktiv (HW3); kein Bewegungsstatus.
bool Muninn_isDriverEnabled();
// Atomarer Bewegungs-Snapshot: true waehrend einer STEP-Fahrt.
bool Muninn_isMoving();
// 0=Stillstand, 1=physisch LINKS/G39, 2=physisch RECHTS/G36.
// Liest die reale DIR-Ausgabe nur bei aktiver Bewegung, ohne ISR-Zugriff.
uint8_t Muninn_motionDirectionSnapshot();
// Simulierte Position nur im Testmodus FY_MOTOR_BENCH_TEST.
// Sonst Nano POSITION_UNKNOWN (0x00007FFF).
uint32_t Muninn_positionSnapshot();
// Referenzstatus: 0=UNBEKANNT, 1=REAL, 2=TESTSIMULATION.
uint8_t Muninn_referenceStatusSnapshot();
// Gepackter Gleisstatus: Ziel | (letztes bestaetigtes Gleis << 8) | (Status << 16).
uint32_t Muninn_trackSnapshot();
// ID-spezifischer atomarer Parameterwert. False bei ungueltigem/fehlendem Wert.
bool Muninn_getParamSnapshot(uint8_t id, uint32_t& value);
// NVM-Fehlermaske / Gueltigkeitsmaske in einem atomaren uint16-Snapshot.
uint16_t Muninn_nvmStatusSnapshot();
// Atomarer Motorstatus: IDLE=0, CONF=1, REFRUN=2, MOVING=3, STOPPED=4, ERROR=5.
MotorState_t Muninn_motorState();

/** @brief Initialisiert Motor-Hardware und STEP-Timer vor dem Taskstart. */
void Muninn_initMotor();
/** @brief Startet die Muninn-Motor-Task auf Core 1.
 *  @param context Gueltiger gemeinsamer Kontext mit den FreeRTOS-Queues.
 *  @return Ergebnis der FreeRTOS-Task-Erstellung.
 */
BaseType_t Muninn_startMotor(BringupContext* context);
}
