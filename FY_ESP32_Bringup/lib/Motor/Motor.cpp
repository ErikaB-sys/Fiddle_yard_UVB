#include "Motor.h"
#include "StepTimer.h"
#include "FY_BenchPosition.h"
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

// Motor state transitions run only in Muninn (core 1). ISR status is
// consumed in the task, never used for unsolicited UART telegrams.
void Motor::onGoEnabled() {
    if (!_StopActive && _State == MotorState_t::STOPPED)
        _State = MotorState_t::IDLE;
}
void Motor::onStop(bool latched) {
    _StopActive = latched;
    _ReferenceValid = false;
    _State = MotorState_t::STOPPED;
    // STOP / GO OFF must never invent a new reference position.
    invalidateBenchPosition();
}
void Motor::onMoveStarted() {
    if (!_StopActive && _State == MotorState_t::IDLE)
        _State = MotorState_t::MOVING;
}
void Motor::onMoveFinished() {
    if (_State == MotorState_t::MOVING)
        _State = MotorState_t::IDLE;
}
void Motor::onProfileError() {
    _ReferenceValid = false;
    invalidateBenchPosition();
    _State = MotorState_t::ERROR;
}
MotorCommandResult_t Motor::prepareReference(uint8_t mode) {
    if (mode > 1U) return MotorCommandResult_t::INVALID_DATA;
    if (_StopActive || _State != MotorState_t::IDLE)
        return MotorCommandResult_t::STOP_ACTIVE;
#if FY_MOTOR_BENCH_TEST
    // Only short REF 00 is simulated. Long REF and all real REF remain off.
    if (mode == 0U) return MotorCommandResult_t::ACCEPTED;
#endif
    return MotorCommandResult_t::STOP_ACTIVE;
}
void Motor::invalidateBenchPosition() {
    _BenchReferenceValid = false;
    _BenchPositionValid = false;
    _MotionPositionTracked = false;
    _MotionStepCount = 0U;
}
void Motor::resetBenchPosition() {
#if FY_MOTOR_BENCH_TEST
    // Explicit fake short reference: assign measured HW_TEST coordinate 1000.
    // Never claim a *physical* reference without sensor confirmation.
    _BenchPosition = FY_BENCH_REFERENCE_POSITION;
    _BenchPositionValid = true;
    _BenchReferenceValid = true;
    _MotionPositionTracked = false;
    _MotionStartPosition = _BenchPosition;
    _MotionStepCount = 0U;
#else
    invalidateBenchPosition();
#endif
}

MotorCommandResult_t Motor::prepareSetTrack(uint8_t track, MoveRequest& request) {
#if FY_MOTOR_BENCH_TEST
    uint16_t target = 0U;
    if (!benchTrackPosition(track, target))
        return MotorCommandResult_t::INVALID_DATA;
    if (!benchReferenceValid())
        return MotorCommandResult_t::STOP_ACTIVE;
    // Reuse existing position prepare / six-phase profile executor.
    return prepareSetPosition(target, request);
#else
    (void)track;
    (void)request;
    return MotorCommandResult_t::STOP_ACTIVE;
#endif
}

MotorCommandResult_t Motor::prepareSetPosition(uint16_t target,
                                                MoveRequest& request) {
    if (_StopActive || _State != MotorState_t::IDLE)
        return MotorCommandResult_t::STOP_ACTIVE;
#if FY_MOTOR_BENCH_TEST
    // A synthetic position is intentionally DISTINCT from real REFRUN.
    if (!_BenchPositionValid)
        return MotorCommandResult_t::STOP_ACTIVE;
    if (target < FY_BENCH_MIN_POSITION ||
        target > FY_BENCH_MAX_POSITION)
        return MotorCommandResult_t::INVALID_DATA;
    const uint16_t steps = benchAbsoluteDistance(_BenchPosition, target);
    if (steps > FY_BENCH_MAX_ABSOLUTE_MOVE)
        return MotorCommandResult_t::INVALID_DATA;
    if (steps != 0U &&
        !benchCanMove(_BenchPosition, steps, target > _BenchPosition))
        return MotorCommandResult_t::INVALID_DATA;
    request.mode = MoveMicrostep::QUARTER;
    request.directionRight = target > _BenchPosition;
    request.steps = steps;
    if (steps > 0U &&
        (!calcProfile(steps) || !isProfileValid(steps)))
        return MotorCommandResult_t::INVALID_DATA;
    return MotorCommandResult_t::ACCEPTED;
#else
    // Real track/FY mode requires physical reference and checked limits.
    (void)target;
    (void)request;
    return MotorCommandResult_t::STOP_ACTIVE;
#endif
}

void Motor::onMotionAccepted(const MoveRequest& request) {
#if FY_MOTOR_BENCH_TEST
    // Quarter-step positions are meaningful only while resolution stays 1/4.
    _MotionPositionTracked =
        _BenchPositionValid && request.mode == MoveMicrostep::QUARTER &&
        benchCanMove(_BenchPosition, request.steps, request.directionRight);
    _MotionStartPosition = _BenchPosition;
    _MotionStepCount = request.steps;
    _MotionRight = request.directionRight;
    if (!_MotionPositionTracked)
        invalidateBenchPosition();
#else
    (void)request;
#endif
}

void Motor::onMotionProgress(uint32_t completedSteps) {
#if FY_MOTOR_BENCH_TEST
    if (!_MotionPositionTracked || !_BenchPositionValid) return;
    const uint32_t done = completedSteps > _MotionStepCount
        ? _MotionStepCount : completedSteps;
    const int32_t pos = static_cast<int32_t>(_MotionStartPosition) +
        (_MotionRight ? static_cast<int32_t>(done)
                      : -static_cast<int32_t>(done));
    if (pos < FY_BENCH_MIN_POSITION ||
        pos > FY_BENCH_MAX_POSITION) {
        invalidateBenchPosition();
        return;
    }
    _BenchPosition = static_cast<uint16_t>(pos);
#else
    (void)completedSteps;
#endif
}
bool Motor::isProfileValid(uint16_t distance) const {
    if (!_TimerValid || distance == 0U) return false;
    uint32_t sum = 0U;
    for (const auto& phase : Motor_profil) sum += phase.Steps;
    return sum == distance;
}

// Parameter validation is kept in Motor (prepare path), not in the ISR.
MotorCommandResult_t Motor::prepareMove(const uint8_t* data,
                                        MoveRequest& request) {
    if (!decodeMovePayload(data, request))
        return MotorCommandResult_t::INVALID_DATA;
    // Guard the virtual position against overflow/underflow.
#if FY_MOTOR_BENCH_TEST
    if (_BenchPositionValid && request.mode == MoveMicrostep::QUARTER &&
        !benchCanMove(_BenchPosition, request.steps, request.directionRight))
        return MotorCommandResult_t::INVALID_DATA;
#endif
    // Prepare all six Nano phases; the STEP ISR now executes those
    // phases, but the new dynamic hardware timer still needs HW testing.
    if (!calcProfile(request.steps) || !isProfileValid(request.steps))
        return MotorCommandResult_t::INVALID_DATA;
    return MotorCommandResult_t::ACCEPTED;
}

namespace {
// Only Muninn writes this; Huginn reads through a C++ atomic snapshot.
std::atomic<bool> driverEnabled{false};
std::atomic<bool> motorMoving{false};
std::atomic<uint8_t> motorStateSnapshot{static_cast<uint8_t>(MotorState_t::STOPPED)};
// Wire marker bit 16 denotes BENCH-SIMULATED position; 0x7FFF alone
// retains its original Nano meaning (POSITION_UNKNOWN).
constexpr uint32_t kPositionUnknown = 0x00007FFFU;
constexpr uint32_t kPositionBenchFlag = 0x00010000U;
std::atomic<uint32_t> positionSnapshot{kPositionUnknown};
std::atomic<uint8_t> referenceStatusSnapshot{0U}; // 0 none, 1 real, 2 fake
std::atomic<uint32_t> trackSnapshot{
    static_cast<uint32_t>(TRACK_STATUS_ERROR) << 16U
}; // target, confirmed, status encoded atomically

void publishPosition(const Motor& motor) {
#if FY_MOTOR_BENCH_TEST
    const uint32_t position = motor.benchPositionValid()
        ? (kPositionBenchFlag | static_cast<uint32_t>(motor.benchPosition()))
        : kPositionUnknown;
    positionSnapshot.store(position, std::memory_order_release);
#else
    (void)motor;
    positionSnapshot.store(kPositionUnknown, std::memory_order_release);
#endif
}

void publishReference(const Motor& motor) {
    const uint8_t status = motor.referenced() ? 1U :
        (motor.benchReferenceValid() ? 2U : 0U);
    referenceStatusSnapshot.store(status, std::memory_order_release);
}

void publishTrack(uint8_t target, uint8_t confirmed, uint8_t status) {
    const uint32_t packed = static_cast<uint32_t>(target) |
        (static_cast<uint32_t>(confirmed) << 8U) |
        (static_cast<uint32_t>(status) << 16U);
    trackSnapshot.store(packed, std::memory_order_release);
}

// The six-phase profile now sets the STEP frequency in the timer ISR.

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

uint32_t Muninn_positionSnapshot() {
    return positionSnapshot.load(std::memory_order_acquire);
}

uint8_t Muninn_referenceStatusSnapshot() {
    return referenceStatusSnapshot.load(std::memory_order_acquire);
}

uint32_t Muninn_trackSnapshot() {
    return trackSnapshot.load(std::memory_order_acquire);
}

MotorState_t Muninn_motorState() {
    return static_cast<MotorState_t>(motorStateSnapshot.load(std::memory_order_acquire));
}

void Muninn_initMotorMock() {
    // G32 (!SLEEP + !RESET) is held LOW during the entire startup.
    // The A4988 microstep inputs are configured while ENABLE is HIGH.
    digitalWrite(MOTOR_ENABLE_PIN, HIGH);
    digitalWrite(MOTOR_PWM_PIN, LOW);
    pinMode(MOTOR_MS0_PIN, OUTPUT);
    pinMode(MOTOR_MS1_PIN, OUTPUT);
    pinMode(MOTOR_MS2_PIN, OUTPUT);
#if FY_MOTOR_BENCH_TEST
    applyMicrostepMode(MoveMicrostep::QUARTER);
#else
    applyMicrostepMode(MoveMicrostep::SIXTEENTH);
#endif
    // Timer is armed ONLY when a bounded motion test is accepted.
    (void)stepTimerBegin();
#if FY_STATUS_LED_PIN >= 0
    pinMode(FY_STATUS_LED_PIN, OUTPUT);
    setLed(false);
#endif
    // HW3: Sleep/Reset is solely a power-on/boot interlock.
    // Wake the A4988 once, after all control lines have known states.
    // No UART or Muninn task exists yet; delay here cannot block commands.
    digitalWrite(MOTOR_SLEEP_PIN, HIGH);
    delayMicroseconds(1500U); // A4988 tWAKE >= 1 ms, outside timer ISR.
}

void Muninn_motorMockTask(void* arg) {
    auto* context = static_cast<BringupContext*>(arg);
    CommandMessage cmd{};
    Motor motor{};
    bool ledState = false;
    // POR/Reset is always locked. Only an explicit STOP AA 55 releases it.
    bool stopActive = true; // Mirrors Motor state for the existing STOP path.
    uint8_t targetTrack = 0U; // 0 = unknown, only Muninn changes these.
    uint8_t confirmedTrack = 0U;
#if FY_MOTOR_BENCH_TEST
    MoveMicrostep activeMicrostepMode = MoveMicrostep::QUARTER;
#endif

    // POR STOP stays latched; the driver is awake after init but disabled.
    // !SLEEP/!RESET (G32) stays HIGH throughout normal GO/STOP operation.
    digitalWrite(MOTOR_PWM_PIN, LOW);
    digitalWrite(MOTOR_ENABLE_PIN, HIGH);
    driverEnabled.store(false, std::memory_order_release);
    motorMoving.store(false, std::memory_order_release);
    // No fake reference at boot. REFERENCE 00 must be explicitly requested.
    publishPosition(motor);
    publishReference(motor);
    publishTrack(0U, 0U, TRACK_STATUS_ERROR);
    motorStateSnapshot.store(static_cast<uint8_t>(motor.state()), std::memory_order_release);

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
            // First disable the driver, then stop STEP.
            // !SLEEP remains HIGH; STOP never resets the A4988 translator.
            // STOP and release both remain non-resuming, Nano semantics.
            digitalWrite(MOTOR_ENABLE_PIN, HIGH);
            driverEnabled.store(false, std::memory_order_release);
            stepTimerStop();
            motorMoving.store(false, std::memory_order_release);
            stopActive = activate;
            motor.onStop(activate);
            targetTrack = 0U;
            confirmedTrack = 0U;
            publishPosition(motor);
            publishReference(motor);
            publishTrack(0U, 0U, TRACK_STATUS_ERROR);
            motorStateSnapshot.store(static_cast<uint8_t>(motor.state()), std::memory_order_release);

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
            motor.onMotionProgress(pulsesDone);
            motorMoving.store(false, std::memory_order_release);
            motor.onMoveFinished();
            publishPosition(motor);
            publishReference(motor);
            if (targetTrack != 0U && motor.benchReferenceValid()) {
                confirmedTrack = targetTrack;
                publishTrack(targetTrack, confirmedTrack, TRACK_STATUS_REACHED);
            }
            motorStateSnapshot.store(static_cast<uint8_t>(motor.state()), std::memory_order_release);
        } else if (motorMoving.load(std::memory_order_acquire)) {
            // Position is a core-1 snapshot, never read straight from UART/ISR.
            motor.onMotionProgress(stepTimerCompletedSteps());
            publishPosition(motor);
            publishReference(motor);
        }

        if (xQueueReceive(context->commands, &cmd, pdMS_TO_TICKS(2)) != pdTRUE)
            continue;

        // Nano GO/STOP enable semantics remain; only the explicit bench
        // MOVE handler below may start a bounded STEP train.
        if (cmd.id == CMD_GO &&
            cmd.data[0] == 0x55U && cmd.data[1] == 0xAAU) {
            // GO 55 AA: de-energize; SLEEP remains HIGH.
            // STOP latch is not affected.
            digitalWrite(MOTOR_ENABLE_PIN, HIGH);
            driverEnabled.store(false, std::memory_order_release);
            stepTimerStop();
            motorMoving.store(false, std::memory_order_release);
            motor.onStop(stopActive);
            targetTrack = 0U;
            confirmedTrack = 0U;
            publishPosition(motor);
            publishReference(motor);
            publishTrack(0U, 0U, TRACK_STATUS_ERROR);
            motorStateSnapshot.store(static_cast<uint8_t>(motor.state()), std::memory_order_release);
            emit(context, cmd, true, cmd.id);
        } else if (cmd.id == CMD_GO &&
                   cmd.data[0] == 0xAAU && cmd.data[1] == 0x55U) {
            // GO AA 55: idempotent enable, without generating any STEP.
            // Never overwrite STEP while an already-accepted train is running.
            if (stopActive) {
                emit(context, cmd, false, nackData(NackReason::NOT_READY));
            } else {
                if (!driverEnabled.load(std::memory_order_acquire)) {
                    // HW3 driver was awakened in Muninn_initMotorMock()
                    // before task creation; STOP latch governs release.
                    digitalWrite(MOTOR_PWM_PIN, LOW);
                    digitalWrite(MOTOR_ENABLE_PIN, LOW); // active LOW
                    driverEnabled.store(true, std::memory_order_release);
                }
                motor.onGoEnabled();
                motorStateSnapshot.store(static_cast<uint8_t>(motor.state()), std::memory_order_release);
                emit(context, cmd, true, cmd.id);
            }
#if FY_MOTOR_BENCH_TEST
        } else if (cmd.id == CMD_REFERENCE) {
            // BENCH ONLY: explicit simulated short REFRUN without motor motion.
            // REFERENCE 01 (long) stays NOT_READY, never fake real sensors.
            if (stopActive || !driverEnabled.load(std::memory_order_acquire)) {
                emit(context, cmd, false, nackData(NackReason::NOT_READY));
            } else if (stepTimerRunning() || motor.state() != MotorState_t::IDLE) {
                emit(context, cmd, false, nackData(NackReason::BUSY));
            } else {
                const MotorCommandResult_t prep = motor.prepareReference(cmd.data[0]);
                if (prep != MotorCommandResult_t::ACCEPTED) {
                    emit(context, cmd, false, nackData(NackReason::NOT_READY));
                } else {
                    motor.resetBenchPosition(); // FAKE: coordinate 1000; no STEP.
                    targetTrack = 0U;
                    confirmedTrack = 0U;
                    publishPosition(motor);
                    publishReference(motor);
                    publishTrack(0U, 0U, TRACK_STATUS_ERROR);
                    emit(context, cmd, true, cmd.id);
                }
            }
        } else if (cmd.id == CMD_MOVE || cmd.id == CMD_SET_POSITION ||
                   cmd.id == CMD_SET_TRACK) {
            // BENCH ONLY. SET_TRACK uses physically measured HW_TEST positions,
            // but the reference is synthetic and MUST remain visibly fake.
            if (stopActive || !driverEnabled.load(std::memory_order_acquire)) {
                emit(context, cmd, false, nackData(NackReason::NOT_READY));
            } else if (stepTimerRunning() ||
                       motor.state() == MotorState_t::MOVING) {
                emit(context, cmd, false, nackData(NackReason::BUSY));
            } else {
                MoveRequest request{};
                const MotorCommandResult_t prep =
                    (cmd.id == CMD_MOVE) ? motor.prepareMove(cmd.data, request) :
                    (cmd.id == CMD_SET_TRACK) ? motor.prepareSetTrack(cmd.data[0], request) :
                    motor.prepareSetPosition(
                        static_cast<uint16_t>(cmd.data[0]) |
                        (static_cast<uint16_t>(cmd.data[1]) << 8U), request);

                if (prep != MotorCommandResult_t::ACCEPTED) {
                    emit(context, cmd, false, nackData(
                        prep == MotorCommandResult_t::INVALID_DATA
                        ? NackReason::INVALID_PARAMETER
                        : NackReason::NOT_READY));
                } else if (request.steps == 0U) {
                    // Already at the requested position. No STEP pulses.
                    if (cmd.id == CMD_SET_TRACK) {
                        targetTrack = cmd.data[0];
                        confirmedTrack = targetTrack;
                        publishTrack(targetTrack, confirmedTrack, TRACK_STATUS_REACHED);
                    } else {
                        targetTrack = 0U;
                        confirmedTrack = 0U;
                        publishTrack(0U, 0U, TRACK_STATUS_ERROR);
                    }
                    publishReference(motor);
                    emit(context, cmd, true, cmd.id);
                } else {
                    // Change microstepping only when the STEP timer is idle.
                    if (request.mode != activeMicrostepMode) {
                        applyMicrostepMode(request.mode);
                        activeMicrostepMode = request.mode;
                    }
                    // Nano ACC1,ACC2,KONST,BRE1,BRE2,POSI prepared in
                    // Motor::calcProfile(). Timer alone owns the ISR copy.
                    StepProfileSegment stages[STEP_PROFILE_PHASES]{};
                    const auto& parameters = motor.profileParams();
                    const uint16_t rampEvery[STEP_PROFILE_PHASES]{
                        parameters.Acc1RampInterval,
                        parameters.Acc2RampInterval,
                        1U,
                        parameters.Bre1RampInterval,
                        parameters.Bre2RampInterval,
                        1U
                    };
                    for (uint8_t i = 0U; i < STEP_PROFILE_PHASES; ++i) {
                        stages[i].steps = motor.profile()[i].Steps;
                        stages[i].accel = motor.profile()[i].Accel;
                        stages[i].rampEvery = rampEvery[i];
                    }
                    if (!stepTimerStartProfile(request.steps,
                                               request.directionRight,
                                               stages)) {
                        // Invalid profile / unavailable timer backend.
                        // Never acknowledge a fixed-speed surrogate as
                        // successful execution of a ramp command.
                        emit(context, cmd, false,
                             nackData(NackReason::NOT_READY));
                    } else {
                        motor.onMotionAccepted(request);
                        motorMoving.store(true, std::memory_order_release);
                        motor.onMoveStarted();
                        if (cmd.id == CMD_SET_TRACK && motor.benchReferenceValid()) {
                            targetTrack = cmd.data[0];
                            // Last confirmed track is no longer guaranteed once moving.
                            confirmedTrack = 0U;
                            publishTrack(targetTrack, 0U, TRACK_STATUS_MOVING);
                        } else {
                            targetTrack = 0U;
                            confirmedTrack = 0U;
                            publishTrack(0U, 0U, TRACK_STATUS_ERROR);
                        }
                        publishPosition(motor);
                        publishReference(motor);
                        motorStateSnapshot.store(
                            static_cast<uint8_t>(motor.state()),
                            std::memory_order_release);
                        emit(context, cmd, true, cmd.id);
                    }
                }
            }
#endif
        } else {
            // Real REFERENCE and SET_TRACK require sensors/calibration.
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
