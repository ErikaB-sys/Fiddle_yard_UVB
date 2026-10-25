#pragma once
#include "FY_Bringup.h"
#include "FY_Move.h"

namespace fy {
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
    int16_t Acc1Accel;
    int16_t Acc2Accel;
    int16_t Bre1Accel;
    int16_t Bre2Accel;
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
    bool calcProfile(uint16_t distance);
    // Parameter preparation for unreferenced commissioning moves.
    // Motor operational state (GO, STOP, MOVING) is still owned by Muninn.
    MotorCommandResult_t prepareMove(const uint8_t* data,
                                     MoveRequest& request);
    // Staged Motor state machine; ownership remains solely on Muninn/core 1.
    MotorState_t state() const { return _State; }
    bool referenced() const { return _ReferenceValid; }
    void onGoEnabled();
    void onStop(bool latched);
    void onMoveStarted();
    void onMoveFinished();
    void onProfileError();
    void onReferenceStarted();
    void onReferenceFinished(bool success);
    MotorCommandResult_t prepareReference(uint8_t mode);
    // Synthetic short REF (without motion) is BENCH ONLY, never sets _ReferenceValid.
    void resetBenchPosition();  // Called exclusively after accepted fake REFERENCE 00.
    void invalidateBenchPosition();
    bool benchPositionValid() const { return _BenchPositionValid; }
    bool benchReferenceValid() const { return _BenchReferenceValid && _BenchPositionValid; }
    uint16_t benchPosition() const { return _BenchPosition; }
    MotorCommandResult_t prepareSetTrack(uint8_t track, MoveRequest& request);
    MotorCommandResult_t prepareSetPosition(uint16_t target,
                                             MoveRequest& request);
    void onMotionAccepted(const MoveRequest& request);
    void onMotionProgress(uint32_t completedSteps);
    bool isProfileValid(uint16_t distance) const;
    const Profilelement_t* profile() const { return Motor_profil; }
    const MotorProfileParam_t& profileParams() const { return _ProfileParam; }
private:
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
    MotorProfileParam_t _ProfileParam{160,400,100,100,-2,-1,+1,+2,1,2,2,1};
    Profilelement_t Motor_profil[6]{{100,-2},{100,-1},{200,0},{100,+1},{100,+2},{10,0}};
};

// Motor state-machine skeleton is active for MOVE/GO/STOP; complete Nano
// reference flow remains pending. The six-phase timer is coded but not yet HW-tested.
// Thread-safe driver-enable snapshot, published by Muninn (core 1).
// GPIO25/A4988 ENABLE is active LOW (HW3). Not a movement status.
bool Muninn_isDriverEnabled();
// Atomic movement snapshot; true while the bounded STEP train is running.
bool Muninn_isMoving();
// A simulated quarter-step position only in FY_MOTOR_BENCH_TEST.
// Otherwise returns Nano POSITION_UNKNOWN (0x00007FFF).
uint32_t Muninn_positionSnapshot();
// Explicit reference truth source: 0=UNKNOWN, 1=REAL, 2=BENCH_FAKE.
uint8_t Muninn_referenceStatusSnapshot();
// Packed STATUS_Track payload: target | (last confirmed << 8) | (status << 16).
uint32_t Muninn_trackSnapshot();
// Atomic status snapshot: IDLE=0, CONF=1, REFRUN=2, MOVING=3, STOPPED=4, ERROR=5.
MotorState_t Muninn_motorState();

void Muninn_initMotorMock();
BaseType_t Muninn_startMotorMock(BringupContext* context);
}
