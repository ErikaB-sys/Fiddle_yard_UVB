#include "Motor.h"
#include "StepTimer.h"
#include "HW_Config.h"
#include "Error.h"
#include <freertos/task.h>
#include <atomic>

#ifndef FY_MOTOR_BENCH_TEST
#define FY_MOTOR_BENCH_TEST 0
#endif

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

// Parameter validation is kept in Motor (prepare path), not in the ISR.
MotorCommandResult_t Motor::prepareMove(const uint8_t* data,
                                        MoveRequest& request) const {
    return decodeMovePayload(data, request)
        ? MotorCommandResult_t::ACCEPTED
        : MotorCommandResult_t::INVALID_DATA;
}

namespace {
// Only Muninn writes this; Huginn reads through a C++ atomic snapshot.
std::atomic<bool> driverEnabled{false};
std::atomic<bool> motorMoving{false};

// Bench-only fixed pulse frequency; the number of steps comes from MOVE.
constexpr uint16_t kBenchStepHz = 120U;

void applyMicrostepMode(MoveMicrostep mode) {
    // A4988 MS1/MS2/MS3 truth table. Apply only while STEP is stopped.
    // Mode changes can invalidate a physical absolute position; bench only.
    constexpr uint8_t masks[] = {0x00U, 0x01U, 0x02U, 0x03U, 0x07U};
    const uint8_t value = masks[static_cast<uint8_t>(mode)];
    digitalWrite(MOTOR_MS0_PIN, (value & 0x01U) ? HIGH : LOW);
    digitalWrite(MOTOR_MS1_PIN, (value & 0x02U) ? HIGH : LOW);
    digitalWrite(MOTOR_MS2_PIN, (value & 0x04U) ? HIGH : LOW);
}

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
    (void)xQueueSend(context->events, &event, portMAX_DELAY);
}
} // anonymous namespace

bool Muninn_isDriverEnabled() {
    return driverEnabled.load(std::memory_order_acquire);
}

bool Muninn_isMoving() {
    return motorMoving.load(std::memory_order_acquire);
}

void Muninn_initMotorMock() {
    // The A4988 microstep inputs are configured while ENABLE is HIGH.
    digitalWrite(MOTOR_ENABLE_PIN, HIGH);
    digitalWrite(MOTOR_PWM_PIN, LOW);
    pinMode(MOTOR_MS0_PIN, OUTPUT);
    pinMode(MOTOR_MS1_PIN, OUTPUT);
    pinMode(MOTOR_MS2_PIN, OUTPUT);
#if FY_MOTOR_BENCH_TEST
    applyMicrostepMode(MoveMicrostep::HALF);
#else
    applyMicrostepMode(MoveMicrostep::SIXTEENTH);
#endif
    // Timer is armed ONLY when a bounded motion test is accepted.
    (void)stepTimerBegin();
#if FY_STATUS_LED_PIN >= 0
    pinMode(FY_STATUS_LED_PIN, OUTPUT);
    setLed(false);
#endif
}

void Muninn_motorMockTask(void* arg) {
    auto* context = static_cast<BringupContext*>(arg);
    CommandMessage cmd{};
    Motor motor{};
    bool ledState = false;
    bool stopActive = false;
#if FY_MOTOR_BENCH_TEST
    MoveMicrostep activeMicrostepMode = MoveMicrostep::HALF;
#endif

    // Begin with the motor driver OFF; no STEP train starts automatically.
    digitalWrite(MOTOR_PWM_PIN, LOW);
    digitalWrite(MOTOR_ENABLE_PIN, HIGH);
    driverEnabled.store(false, std::memory_order_release);
    motorMoving.store(false, std::memory_order_release);

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
            // First disable the driver, THEN stop STEP generation.
            // STOP and release both remain non-resuming, Nano semantics.
            digitalWrite(MOTOR_ENABLE_PIN, HIGH);
            driverEnabled.store(false, std::memory_order_release);
            stepTimerStop();
            motorMoving.store(false, std::memory_order_release);
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

        // A completed pulse train is a motor event, not a new UART ACK.
        // Completion retains coil holding torque until explicit GO off.
        uint32_t pulsesDone = 0U;
        if (stepTimerTakeDone(pulsesDone)) {
            motorMoving.store(false, std::memory_order_release);
            (void)pulsesDone; // Bench position is intentionally not referenced.
        }

        if (xQueueReceive(context->commands, &cmd, pdMS_TO_TICKS(2)) != pdTRUE)
            continue;

        // Nano GO/STOP enable semantics remain; only the explicit bench
        // MOVE handler below may start a bounded STEP train.
        if (cmd.id == CMD_GO &&
            cmd.data[0] == 0x55U && cmd.data[1] == 0xAAU) {
            // GO 55 AA: de-energize the driver; do NOT clear the STOP latch.
            digitalWrite(MOTOR_ENABLE_PIN, HIGH);
            driverEnabled.store(false, std::memory_order_release);
            stepTimerStop();
            motorMoving.store(false, std::memory_order_release);
            emit(context, cmd, true, cmd.id);
        } else if (cmd.id == CMD_GO &&
                   cmd.data[0] == 0xAAU && cmd.data[1] == 0x55U) {
            // GO AA 55: idempotent enable, without generating any STEP.
            // Never overwrite STEP while an already-accepted train is running.
            if (stopActive) {
                emit(context, cmd, false, nackData(NackReason::NOT_READY));
            } else {
                if (!driverEnabled.load(std::memory_order_acquire)) {
                    digitalWrite(MOTOR_PWM_PIN, LOW);
                    digitalWrite(MOTOR_ENABLE_PIN, LOW); // active LOW
                    driverEnabled.store(true, std::memory_order_release);
                }
                emit(context, cmd, true, cmd.id);
            }
#if FY_MOTOR_BENCH_TEST
        } else if (cmd.id == CMD_MOVE) {
            // TEMPORARY BENCH MODE ONLY: no reference or track movement.
            // STEP count and direction come from MOVE, never from a mock
            // hardcoded left/right command. No new job while running.
            MoveRequest request{};
            if (motor.prepareMove(cmd.data, request) !=
                MotorCommandResult_t::ACCEPTED) {
                emit(context, cmd, false,
                     nackData(NackReason::INVALID_PARAMETER));
            } else if (stopActive ||
                       !driverEnabled.load(std::memory_order_acquire)) {
                emit(context, cmd, false, nackData(NackReason::NOT_READY));
            } else if (stepTimerRunning()) {
                emit(context, cmd, false, nackData(NackReason::BUSY));
            } else {
                // A4988 samples a new MS mode at the NEXT rising STEP edge.
                // For this loose, unreferenced test motor only: do not assume
                // position continuity when the microstep mode changes.
                if (request.mode != activeMicrostepMode) {
                    applyMicrostepMode(request.mode);
                    activeMicrostepMode = request.mode;
                }
                if (!stepTimerStart(request.steps, request.directionRight,
                                    kBenchStepHz)) {
                    emit(context, cmd, false, nackData(NackReason::BUSY));
                } else {
                    motorMoving.store(true, std::memory_order_release);
                    emit(context, cmd, true, cmd.id);
                }
            }
#endif
        } else {
            // Reference/track/position still require the real Motor state.
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
