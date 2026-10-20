#pragma once
#include "FY_Bringup.h"

namespace fy {
// Ported verbatim in meaning from Nano Motor.h: motor-owned job/payload.
struct MotorJob_t {
    bool valid;
    uint8_t cmd;
    uint8_t data[MAX_COMMAND_LENGTH - 1];
};

// Nano motion model types. No ESP32 timer/ISR backend is attached yet.
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
    const Profilelement_t* profile() const { return Motor_profil; }
private:
    bool _TimerValid = false;
    MotorProfileParam_t _ProfileParam{160,400,100,100,-2,-1,+1,+2,1,1,1,1};
    Profilelement_t Motor_profil[6]{{100,-2},{100,-1},{200,0},{100,+1},{100,+2},{10,0}};
};

// Bring-up still uses the motor mock: GO/STOP drive ENABLE, and calcProfile()
// is ported, but Nano setJob(), prepare...() and the motor timer/ISR are pending.
// Thread-safe driver-enable snapshot, published by Muninn (core 1).
// GPIO27/A4988 ENABLE is active LOW. This is not a movement status.
bool Muninn_isDriverEnabled();
// Atomic movement snapshot; true while the bounded STEP train is running.
bool Muninn_isMoving();

void Muninn_initMotorMock();
BaseType_t Muninn_startMotorMock(BringupContext* context);
}
