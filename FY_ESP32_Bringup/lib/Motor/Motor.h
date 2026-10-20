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

// Bring-up still uses the motor mock; real Nano setJob(), prepare...(),
// calcProfile() and ISR require the ESP32 motor state / timer HAL port.
void Muninn_initMotorMock();
BaseType_t Muninn_startMotorMock(BringupContext* context);
}
