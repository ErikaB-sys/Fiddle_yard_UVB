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
                                     MoveRequest& request) const;
    // Staged Motor state machine; ownership remains solely on Muninn/core 1.
    MotorState_t state() const { return _State; }
    bool referenced() const { return _ReferenceValid; }
    void onGoEnabled();
    void onStop(bool latched);
    void onMoveStarted();
    void onMoveFinished();
    void onProfileError();
    MotorCommandResult_t prepareReference(uint8_t mode);
    MotorCommandResult_t prepareSetPosition(uint16_t target);
    bool isProfileValid(uint16_t distance) const;
    const Profilelement_t* profile() const { return Motor_profil; }
    const MotorProfileParam_t& profileParams() const { return _ProfileParam; }
private:
    MotorState_t _State = MotorState_t::STOPPED; // POR/STOP latch.
    bool _ReferenceValid = false; // NEVER invent a reference in bench mode.
    bool _StopActive = true;
    bool _TimerValid = false;
    MotorProfileParam_t _ProfileParam{160,400,100,100,-2,-1,+1,+2,1,1,1,1};
    Profilelement_t Motor_profil[6]{{100,-2},{100,-1},{200,0},{100,+1},{100,+2},{10,0}};
};

// Motor state-machine skeleton is active for MOVE/GO/STOP; complete Nano
// reference flow and dynamic six-phase timer executor remain pending.
// Thread-safe driver-enable snapshot, published by Muninn (core 1).
// GPIO27/A4988 ENABLE is active LOW. This is not a movement status.
bool Muninn_isDriverEnabled();
// Atomic movement snapshot; true while the bounded STEP train is running.
bool Muninn_isMoving();
// Atomic status snapshot: IDLE=0, CONF=1, REFRUN=2, MOVING=3, STOPPED=4, ERROR=5.
MotorState_t Muninn_motorState();

void Muninn_initMotorMock();
BaseType_t Muninn_startMotorMock(BringupContext* context);
}
