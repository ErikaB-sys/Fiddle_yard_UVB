#include "Motor.h"
#include <freertos/task.h>

#ifndef FY_STATUS_LED_PIN
#define FY_STATUS_LED_PIN 2
#endif
#ifndef FY_STATUS_LED_ACTIVE_HIGH
#define FY_STATUS_LED_ACTIVE_HIGH 1
#endif

namespace fy {

// Taken from the verified Nano Motor::calcProfile(), unchanged calculation.
bool Motor::calcProfile(uint16_t distance)
{
    // Sehr kurze Strecke:
    // Kein vollstaendiges Profil moeglich.
    // Die gesamte Strecke wird mit V_MIN gefahren.
    if (distance <= (_ProfileParam.PosiMin + _ProfileParam.KonstMin))
    {
        for (uint8_t i = 0; i < 6; ++i)
        {
            Motor_profil[i].Steps = 0;
            Motor_profil[i].Accel = 0;
        }

        Motor_profil[static_cast<uint8_t>(MotorProfile_t::KONST)].Steps =
            distance;

        _TimerValid = true;
        return true;
    }

    for (uint8_t i = 0; i < 6; ++i)
    {
        Motor_profil[i].Steps = 0;
        Motor_profil[i].Accel = 0;
    }

    Motor_profil[static_cast<uint8_t>(MotorProfile_t::POSI)].Steps =
        _ProfileParam.PosiMin;

    Motor_profil[static_cast<uint8_t>(MotorProfile_t::KONST)].Steps =
        _ProfileParam.KonstMin;

    uint16_t remaining =
        distance
        - _ProfileParam.PosiMin
        - _ProfileParam.KonstMin;

    uint16_t outer = remaining / 2;

    if (outer > _ProfileParam.Acc1Steps)
        outer = _ProfileParam.Acc1Steps;

    Motor_profil[static_cast<uint8_t>(MotorProfile_t::ACC1)].Steps = outer;
    Motor_profil[static_cast<uint8_t>(MotorProfile_t::ACC1)].Accel =
        _ProfileParam.Acc1Accel;

    Motor_profil[static_cast<uint8_t>(MotorProfile_t::BRE2)].Steps = outer;
    Motor_profil[static_cast<uint8_t>(MotorProfile_t::BRE2)].Accel =
        _ProfileParam.Bre2Accel;

    remaining -= outer * 2;

    uint16_t inner = remaining / 2;

    if (inner > _ProfileParam.Acc2Steps)
        inner = _ProfileParam.Acc2Steps;

    Motor_profil[static_cast<uint8_t>(MotorProfile_t::ACC2)].Steps = inner;
    Motor_profil[static_cast<uint8_t>(MotorProfile_t::ACC2)].Accel =
        _ProfileParam.Acc2Accel;

    Motor_profil[static_cast<uint8_t>(MotorProfile_t::BRE1)].Steps = inner;
    Motor_profil[static_cast<uint8_t>(MotorProfile_t::BRE1)].Accel =
        _ProfileParam.Bre1Accel;

    remaining -= inner * 2;

    Motor_profil[static_cast<uint8_t>(MotorProfile_t::KONST)].Steps +=
        remaining;

    uint16_t sum = 0;

    for (uint8_t i = 0; i < 6; ++i)
        sum += Motor_profil[i].Steps;

    if (sum < distance)
    {
        Motor_profil[static_cast<uint8_t>(MotorProfile_t::POSI)].Steps +=
            distance - sum;
    }

    _TimerValid = true;
    return true;
}



namespace {
void setLed(bool on) {
#if FY_STATUS_LED_PIN >= 0
    digitalWrite(FY_STATUS_LED_PIN,
                 (on == (FY_STATUS_LED_ACTIVE_HIGH != 0)) ? HIGH : LOW);
#else
    (void)on;
#endif
}

uint8_t nackData(NackReason reason) {
    return static_cast<uint8_t>(
        FY_ERROR_LOCATION_MOTOR | static_cast<uint8_t>(reason));
}

void emit(BringupContext* context, const CommandMessage& cmd,
          bool accepted, uint8_t data) {
    const MotorEvent event{
        cmd.sequence, cmd.id,
        static_cast<uint8_t>(xPortGetCoreID()),
        accepted ? STATUS_ACK : STATUS_NACK,
        data
    };
    (void)xQueueSend(context->events, &event, pdMS_TO_TICKS(10));
}
} // anonymous namespace

void Muninn_initMotorMock() {
#if FY_STATUS_LED_PIN >= 0
    pinMode(FY_STATUS_LED_PIN, OUTPUT);
    setLed(false);
#endif
}

void Muninn_motorMockTask(void* arg) {
    auto* context = static_cast<BringupContext*>(arg);
    CommandMessage cmd{};
    bool ledState = false;
    bool stopActive = false;

    // Motor is NOT initialized. Keep A4988 disabled in this transport test.
    digitalWrite(MOTOR_PWM_PIN, LOW);
    digitalWrite(MOTOR_ENABLE_PIN, HIGH);

    for (;;) {
        // STOP has its own mailbox and is always checked before normal jobs.
        if (xQueueReceive(context->priorityCommands, &cmd, 0) == pdTRUE) {
            const bool activate = cmd.data[0] == 0x55U && cmd.data[1] == 0xAAU;
            const bool release = cmd.data[0] == 0xAAU && cmd.data[1] == 0x55U;
            if (cmd.id != CMD_STOPP || (!activate && !release)) {
                emit(context, cmd, false,
                     nackData(NackReason::INVALID_PARAMETER));
                continue;
            }
            // Both STOP and its release force the driver OFF. Neither
            // can start the motor or restore a reference (Nano semantics).
            digitalWrite(MOTOR_PWM_PIN, LOW);
            digitalWrite(MOTOR_ENABLE_PIN, HIGH);
            stopActive = activate;

            // Cancel any old queued movement requests; every dropped request
            // still receives an explicit negative result.
            CommandMessage pending{};
            while (xQueueReceive(context->commands, &pending, 0) == pdTRUE)
                emit(context, pending, false, nackData(NackReason::NOT_READY));

            emit(context, cmd, true, cmd.id);
            ledState = !ledState;
            setLed(ledState);
            continue;
        }

        if (xQueueReceive(context->commands, &cmd, pdMS_TO_TICKS(2)) != pdTRUE)
            continue;

        // Once the timer ISR and Motor::prepareGo() are ported, these
        // simulations will be replaced by real Motor command results.
        if (cmd.id == CMD_GO &&
            cmd.data[0] == 0x55U && cmd.data[1] == 0xAAU) {
            digitalWrite(MOTOR_PWM_PIN, LOW);
            digitalWrite(MOTOR_ENABLE_PIN, HIGH);
            stopActive = false;
            emit(context, cmd, true, cmd.id); // Safe driver disable.
        } else {
            // All actions requiring a live motor/backend are rejected.
            // Do not announce a successful move/reference/driver enable.
            (void)stopActive;
            emit(context, cmd, false, nackData(NackReason::NOT_READY));
        }

        ledState = !ledState;
        setLed(ledState);
    }
}

BaseType_t Muninn_startMotorMock(BringupContext* context) {
    return xTaskCreatePinnedToCore(
        Muninn_motorMockTask, "Muninn_Motor", 4096,
        context, 3, nullptr, CORE_MUNINN);
}
} // namespace fy
