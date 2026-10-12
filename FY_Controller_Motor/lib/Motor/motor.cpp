// Motor.cpp
// Motor control implementation

#include <Arduino.h>
#include <util/atomic.h>
#include "FY_System.h"
#include "HW_Config.h"
#include "Motor.h"
#include "Protokoll.h"
#include "Error.h"


REF_STATE FY_Refrun = REF_STATE::REF_REQ;

// The motor module owns the measurement data updated by motor execution/ISR.
volatile HWMeasurement measurement{};

 // Shared execution/profile/reference storage adopted from FY_HW_TEST in step 3a.
volatile bool stepRun = false;
volatile bool stepLevel = false;
volatile bool profileRun = false;
volatile bool profileReportPending = false;
volatile bool updateOut = true;
HWISRUiState hw{};

uint16_t profileVMin = 80U;
uint16_t profileVMax = 800U;
volatile uint8_t profileRampCounter = 0U;
Profilelement_t motionProfile[6]{};
volatile uint8_t profileElement = 0U;
volatile uint16_t profileStepsRemaining = 0U;
volatile uint16_t profileTimerValue = 0U;
volatile uint16_t profileMinReloadReached = 65535U;

RefRunState refRunState = RefRunState::IDLE;
bool refRunActive = false;
bool refLongRun = false;
uint16_t refRunVMax = 80U;
uint16_t refBackoffTarget = 0U;
volatile uint16_t refCoarsePosition = 0U;
volatile uint16_t refRightEndPosition = 0U;
volatile uint16_t refPreciseRight = 0U;
volatile uint16_t refPreciseLeft = 0U;
volatile uint16_t refPreciseLength = 0U;
volatile uint16_t refPreciseCenter = 0U;
volatile uint16_t refPreciseRight2 = 0U;
volatile uint16_t refPreciseLeft2 = 0U;
volatile uint16_t refPreciseLength2 = 0U;
volatile uint16_t refPreciseCenter2 = 0U;
volatile uint16_t refApproachTarget = 0U;
volatile uint16_t refShiftTarget = 0U;
volatile uint8_t refMeasurePass = 0U;
volatile bool refCoarseCaptured = false;
volatile bool refPreciseRightCaptured = false;
volatile bool refPreciseCaptured = false;
volatile bool refSecondPreciseCaptured = false;
volatile bool refMeasureArmed = false;
volatile bool refLastState = false;
volatile bool refStartValid = false;
volatile bool leftReferenceApplied = false;
volatile bool rightLengthCaptured = false;

bool load_next_profile_element()
{
    while (profileElement < 6U)
    {
        profileStepsRemaining = motionProfile[profileElement].Steps;
        if (profileStepsRemaining > 0U)
        {
            profileRampCounter = 0U;
            OCR1A = profileTimerValue;
            return true;
        }
        ++profileElement;
    }
    return false;
}

void get_measurement(HWMeasurement* result)
{
    if (result == nullptr)
        return;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        result->position = measurement.position;
        result->refStart = measurement.refStart;
        result->refEnd = measurement.refEnd;
        result->refLength = measurement.refLength;
        result->totalLength = measurement.totalLength;
        result->refValid = measurement.refValid;
    }
}

Motor* Motor::_TimerMotor = nullptr;

MotorProfileParam_t Motor::_ProfileParam =
{
    POS_MIN,                 // PosiMin
    KONST_MIN,               // KonstMin
    100,                     // Acc1Steps
    100,                     // Acc2Steps
    -2,                      // Acc1Accel
    -1,                      // Acc2Accel
    +1,                      // Bre1Accel
    +2,                      // Bre2Accel
    1,                       // Acc1RampInterval
    1,                       // Acc2RampInterval
    1,                       // Bre1RampInterval
    1                        // Bre2RampInterval
};


// -----------------------------------------------------------------------------
// Hilfsfunktionen Timer
// -----------------------------------------------------------------------------

static uint16_t frequency_to_reload(uint16_t frequency)
{
    if (frequency == 0U) return 65535U;
    const uint32_t denominator = 2UL * TIMER_PRESCALER * static_cast<uint32_t>(frequency);
    uint32_t reload = ((TIMER_FREQUENCY / denominator) + (denominator / 2UL)) - 1UL;
    if (reload < 1UL) reload = 1UL;
    if (reload > 65535UL) reload = 65535UL;
    return static_cast<uint16_t>(reload);
}

// -----------------------------------------------------------------------------
// Constructor / Destructor
// -----------------------------------------------------------------------------

Motor::Motor()
{
    _Job.valid = false;
    _JobResult = MotorJobResult_t::NONE;
}


// -----------------------------------------------------------------------------
// Initialization
// -----------------------------------------------------------------------------

bool Motor::begin()
{
    _Dir_pin  = MOTOR_DIR_PIN;
    _Step_pin = MOTOR_PWM_PIN;
    _ENA_pin  = MOTOR_ENABLE_PIN;

    pinMode(_Dir_pin, OUTPUT);
    pinMode(_Step_pin, OUTPUT);
    pinMode(_ENA_pin, OUTPUT);

    // Inputs match the physically verified FY_HW_TEST wiring.
    pinMode(SWITCH_TRIM_LEFT_D, INPUT);
    pinMode(SWITCH_TRIM_RIGHT_D, INPUT);
    pinMode(SWITCH_REF_D, INPUT);
    _LeftLimitInputPort = portInputRegister(digitalPinToPort(SWITCH_TRIM_LEFT_D));
    _RightLimitInputPort = portInputRegister(digitalPinToPort(SWITCH_TRIM_RIGHT_D));
    _ReferenceInputPort = portInputRegister(digitalPinToPort(SWITCH_REF_D));
    _LeftLimitMask = digitalPinToBitMask(SWITCH_TRIM_LEFT_D);
    _RightLimitMask = digitalPinToBitMask(SWITCH_TRIM_RIGHT_D);
    _ReferenceInputMask = digitalPinToBitMask(SWITCH_REF_D);
    if ((_LeftLimitInputPort == nullptr) || (_RightLimitInputPort == nullptr) ||
        (_ReferenceInputPort == nullptr) || (_LeftLimitMask == 0U) ||
        (_RightLimitMask == 0U) || (_ReferenceInputMask == 0U))
        return false;

    // Die Portregister werden einmalig aufgeloest.
    // In der ISR wird damit kein digitalWrite() benoetigt.
    _StepPort = portOutputRegister(digitalPinToPort(_Step_pin));
    _DirPort  = portOutputRegister(digitalPinToPort(_Dir_pin));
    _StepMask = digitalPinToBitMask(_Step_pin);
    _DirMask  = digitalPinToBitMask(_Dir_pin);

    if ((_StepPort == nullptr) || (_DirPort == nullptr) ||
        (_StepMask == 0) || (_DirMask == 0))
    {
        return false;
    }

    // Initiale Hardwarezustaende.
    *_StepPort &= static_cast<uint8_t>(~_StepMask);
    *_DirPort  &= static_cast<uint8_t>(~_DirMask);
    digitalWrite(_ENA_pin, HIGH);       // Treiber deaktiviert

    // Dieses Motor-Objekt ist dasjenige, das Timer1 verwendet.
    _TimerMotor = this;

    Timer1_Init();
    Timer1_Stop();

    _State = MotorState_t::IDLE;
    _StopActive = false;
    _ProfileElement = MotorProfile_t::ACC1;
    _StepsRemaining = 0;
    _TimerValue = TIMER_V_MIN;
    _ProfileRampCounter = 0;
    _StepRun = false;
    _StepLevel = false;
    _ReferenceContinuous = false;
    _ReferenceState = ReferenceState_t::IDLE;
    _ReferenceLastSensorState = ((*_ReferenceInputPort & _ReferenceInputMask) != 0U);
    _TimerValid = false;
    _Position = POSITION_UNKNOWN;
    _TotalLength = 0U;
    _DirectionRight = true;

    // HW_TEST measurement starts at the mechanical midpoint; this is not
    // a validated calibration. The reference run will set refValid later.
    measurement.position = 12500U;
    measurement.refStart = 0U;
    measurement.refEnd = 0U;
    measurement.refLength = 0U;
    measurement.totalLength = 25000U;
    measurement.refValid = false;
    stepRun = false;
    stepLevel = false;
    profileRun = false;
    profileReportPending = false;
    profileElement = 0U;
    profileStepsRemaining = 0U;
    profileTimerValue = frequency_to_reload(profileVMin);
    profileMinReloadReached = profileTimerValue;
    profileRampCounter = 0U;
    _TargetPosition = 0;
    _JobResult = MotorJobResult_t::NONE;

    return true;
}


// -----------------------------------------------------------------------------
// Main update function
// -----------------------------------------------------------------------------

void Motor::Update()
{
    if (_State == MotorState_t::REFRUN && _ReferenceState != ReferenceState_t::IDLE)
    {
        updateReference();
        return;
    }

    // Preserve the existing FY_Motor post-reference handling below.
    if ((false == _Job.valid) && (_State != MotorState_t::REFRUN))
        return;

    switch (_State)
    {
    case MotorState_t::IDLE:
        switch (_Job.cmd)
        {
        case CMD_GO:
            // Timer1_Start();
            break;

        case CMD_REFERENCE:
            if (Prepare())
                _State = MotorState_t::REFRUN;
            else
            {
                _JobResult = MotorJobResult_t::NONE;
                _Job.valid = false;
                _State = MotorState_t::IDLE;
            }
            break;

        case CMD_LEFT:
            if (true == prepareLeft())
                _State = MotorState_t::MOVING;
            else
                _State = MotorState_t::ERROR;
            break;

        case CMD_RIGHT:
            if (true == prepareRight())
                _State = MotorState_t::MOVING;
            else
                _State = MotorState_t::ERROR;
            break;

        case CMD_SET_POSITION:
            if (true == prepareSetPosition())
                _State = MotorState_t::MOVING;
            else
                _State = MotorState_t::ERROR;
            break;

        case CMD_SET_TRACK:
            if (true == prepareSetTrack())
                _State = MotorState_t::MOVING;
            else
                _State = MotorState_t::ERROR;
            break;

        case CMD_SET_PARAM:
            if (true == prepareParameter())
                _State = MotorState_t::CONF;
            else
                _State = MotorState_t::ERROR;
            break;

        default:
            _State = MotorState_t::ERROR;
            break;
        }
        break;

    case MotorState_t::REFRUN:
        if (FY_Refrun == REF_STATE::REF_VALID)
        {
            // Match the HW-test short reference sequence: establish position
            // 1000 at the left end stop, then move to the center track BG3.
            _JobResult = MotorJobResult_t::NONE;
            _TargetPosition = 4757U;
            move2Pos(static_cast<int16_t>(_TargetPosition));

            if (_StepRun || (_Position == _TargetPosition))
                _State = MotorState_t::MOVING;
            else
            {
                _Job.valid = false;
                _State = MotorState_t::ERROR;
            }
        }
        else if (FY_Refrun == REF_STATE::REF_ERROR)
        {
            _Job.valid = false;
            _State = MotorState_t::ERROR;
        }
        break;

    case MotorState_t::CONF:
        if (prepareParameter())
            _State = MotorState_t::IDLE;
        break;

    case MotorState_t::MOVING:
        // The ISR owns the pulse timing. The main loop only resolves its result.
        if (!_StepRun)
        {
            if (_JobResult == MotorJobResult_t::NONE)
                _JobResult = MotorJobResult_t::DONE;
            _Job.valid = false;
            _State = MotorState_t::IDLE;
        }
        break;

    case MotorState_t::STOPPED:
        // New job -> moving, otherwise -> idle
        break;

    case MotorState_t::ERROR:
        break;

    default:
        _State = MotorState_t::ERROR;
        break;
    }
}


// -----------------------------------------------------------------------------
// Movement commands
// -----------------------------------------------------------------------------

void Motor::move2Pos(int16_t targetPosition)
{
    if (FY_Refrun != REF_STATE::REF_VALID || targetPosition < 0 ||
        targetPosition > static_cast<int16_t>(POS_MAX))
        return;

    const uint16_t current = getPosition();
    _TargetPosition = static_cast<uint16_t>(targetPosition);
    _DirectionRight = (_TargetPosition >= current);

    if (_DirPort != nullptr)
    {
        if (_DirectionRight)
            *_DirPort |= _DirMask;
        else
            *_DirPort &= static_cast<uint8_t>(~_DirMask);
    }

    const uint16_t distance = (_TargetPosition >= current)
        ? static_cast<uint16_t>(_TargetPosition - current)
        : static_cast<uint16_t>(current - _TargetPosition);

    if (distance == 0U)
    {
        _JobResult = MotorJobResult_t::DONE;
        _StepRun = false;
        return;
    }

    if (calcProfile(distance))
        Timer1_Start();
}

void Motor::moveSteps(int16_t Steps)
{
    if (Steps == 0)
        return;

    const uint16_t current = getPosition();
    const int32_t target = static_cast<int32_t>(current) + Steps;

    if ((current == POSITION_UNKNOWN) || target < 0 || target > POS_MAX)
        return;

    _TargetPosition = static_cast<uint16_t>(target);
    _DirectionRight = Steps > 0;

    if (_DirPort != nullptr)
    {
        if (_DirectionRight)
            *_DirPort |= _DirMask;
        else
            *_DirPort &= static_cast<uint8_t>(~_DirMask);
    }

    if (calcProfile(static_cast<uint16_t>(Steps > 0 ? Steps : -Steps)))
        Timer1_Start();
}


bool Motor::setJob(MotorJob_t newjob)
{
    if (_State != MotorState_t::IDLE)
        return false;

    const CommandDefinition* definition = nullptr;

    for (const auto& entry : commandDefinitions)
    {
        if (entry.id == newjob.cmd)
        {
            definition = &entry;
            break;
        }
    }

    if (definition == nullptr)
        return false;

    if (definition->requiresReference &&
        FY_Refrun != REF_STATE::REF_VALID)
        return false;

    _Job = newjob;
    _JobResult = MotorJobResult_t::NONE;
    _Job.valid = true;

    return true;
}


// -----------------------------------------------------------------------------
// Stop / emergency
// -----------------------------------------------------------------------------

void Motor::Stop()
{
    // Stoppen beendet nur die STEP-Erzeugung.
    // Der Treiber bleibt aktiviert, damit die Position gehalten wird.
    _StepRun = false;
    _StepLevel = false;

    if (_StepPort != nullptr)
        *_StepPort &= static_cast<uint8_t>(~_StepMask);

    Timer1_Stop();
    _State = MotorState_t::STOPPED;
}

void Motor::Emergency_break()
{
    // Sofortiger Stopp und anschliessendes Deaktivieren des Treibers.
    _StepRun = false;
    _StepLevel = false;

    if (_StepPort != nullptr)
        *_StepPort &= static_cast<uint8_t>(~_StepMask);

    Timer1_Stop();
    digitalWrite(_ENA_pin, HIGH);
    _State = MotorState_t::STOPPED;
}

/**
 * @brief Disable the driver first, then stop STEP generation and invalidate
 *        all state that can no longer be trusted after motor disable.
 */
void Motor::disableAndInvalidate(bool latchStop)
{
    // First action: the driver is disabled, so the motor stops immediately.
    digitalWrite(_ENA_pin, HIGH);

    _StepRun = false;
    _StepLevel = false;
    Timer1_Stop();

    if (_StepPort != nullptr)
        *_StepPort &= static_cast<uint8_t>(~_StepMask);

    _TimerValid = false;
    _ReferenceContinuous = false;
    _StepsRemaining = 0U;
    _Job.valid = false;
    _JobResult = MotorJobResult_t::NONE;
    FY_Refrun = REF_STATE::REF_REQ;
    _Position = POSITION_UNKNOWN;
    _TargetPosition = 0U;
    _State = MotorState_t::STOPPED;

    if (latchStop)
        _StopActive = true;
}

MotorCommandResult_t Motor::prepareStop(const uint8_t* data)
{
    if (data == nullptr)
        return MotorCommandResult_t::INVALID_DATA;

    const uint8_t first = data[0];
    const uint8_t second = data[1];

    // Issue #129: 55 AA activates STOPP; AA 55 releases it.
    if ((first == 0x55U) && (second == 0xAAU))
    {
        disableAndInvalidate(true);
        return MotorCommandResult_t::ACCEPTED;
    }

    if ((first == 0xAAU) && (second == 0x55U))
    {
        // Release also guarantees a disabled driver and stopped timer.
        // It never restores a reference or resumes a previous movement.
        disableAndInvalidate(false);
        _StopActive = false;
        return MotorCommandResult_t::ACCEPTED;
    }

    return MotorCommandResult_t::INVALID_DATA;
}

MotorCommandResult_t Motor::prepareGo(const uint8_t* data)
{
    if (data == nullptr)
        return MotorCommandResult_t::INVALID_DATA;

    const uint8_t first = data[0];
    const uint8_t second = data[1];

    // GO 55 AA switches the motor off; GO AA 55 switches it on.
    if ((first == 0x55U) && (second == 0xAAU))
    {
        disableAndInvalidate(false);
        return MotorCommandResult_t::ACCEPTED;
    }

    if ((first == 0xAAU) && (second == 0x55U))
    {
        if (_StopActive)
            return MotorCommandResult_t::STOP_ACTIVE;

        if ((_State == MotorState_t::MOVING) ||
            (_State == MotorState_t::REFRUN))
            return MotorCommandResult_t::BUSY;

        digitalWrite(_ENA_pin, LOW);
        _State = MotorState_t::IDLE;
        _Job.valid = false;
        _JobResult = MotorJobResult_t::NONE;
        // Enabling the driver does not establish or restore a reference.
        return MotorCommandResult_t::ACCEPTED;
    }

    return MotorCommandResult_t::INVALID_DATA;
}


// -----------------------------------------------------------------------------
// Reference
// -----------------------------------------------------------------------------

bool Motor::Prepare()
{
    if ((_State != MotorState_t::IDLE) || !_Job.valid || (_Job.cmd != CMD_REFERENCE))
        return false;

    // DATA[0]: 0x00 = short, 0x01 = long.
    const uint8_t mode = _Job.data[0];
    if (mode > 0x01U)
        return false;

    // Persistent calibration storage is not implemented yet; short mode
    // is temporarily accepted and establishes the canonical left datum.
    _ReferenceMode = mode;
    _LastReferenceMode = mode;
    _ReferenceLongRun = (mode == 0x01U);
    return Reference();
}

bool Motor::Reference()
{
    if ((_State != MotorState_t::IDLE) || !_Job.valid || (_Job.cmd != CMD_REFERENCE))
        return false;

    _ReferenceState = ReferenceState_t::REF_START;
    _ReferenceLeftEndHit = false;
    _ReferenceRightEndHit = false;
    _ReferenceError = false;
    _ReferenceCoarseCaptured = false;
    _ReferencePreciseRightCaptured = false;
    _ReferencePreciseCaptured = false;
    _ReferenceSecondPreciseCaptured = false;
    _ReferenceMeasureArmed = false;
    _ReferenceStartValid = false;
    _ReferenceMeasurePass = 0U;
    _ReferenceSteps = 0U;
    _ReferenceCoarsePosition = 0U;
    _ReferenceRightEndPosition = 0U;
    _ReferencePreciseRight = 0U;
    _ReferencePreciseLeft = 0U;
    _ReferencePreciseLength = 0U;
    _ReferencePreciseCenter = 0U;
    _ReferencePreciseRight2 = 0U;
    _ReferencePreciseLeft2 = 0U;
    _ReferencePreciseLength2 = 0U;
    _ReferencePreciseCenter2 = 0U;
    _ReferenceApproachTarget = 0U;
    _ReferenceShiftTarget = 0U;
    _Position = REFERENCE_START_POSITION;
    _TargetPosition = REFERENCE_LEFT_POSITION;
    _JobResult = MotorJobResult_t::NONE;
    FY_Refrun = REF_STATE::REFR_EXEC;
    return true;
}

void Motor::startReferenceMotion(bool directionRight, uint16_t frequency)
{
    // Bound each individual leg; a complete long reference consists of
    // several legs and can legitimately exceed REFERENCE_MAX_STEPS in total.
    _ReferenceSteps = 0U;
    _DirectionRight = directionRight;
    if (_DirPort != nullptr)
    {
        if (directionRight)
            *_DirPort |= _DirMask;
        else
            *_DirPort &= static_cast<uint8_t>(~_DirMask);
    }

    _StepLevel = false;
    _StepRun = true;
    _TimerValid = true;
    _TimerValue = frequency_to_reload(frequency);
    OCR1A = _TimerValue;
    TCNT1 = 0;
    digitalWrite(_ENA_pin, LOW);
    if (_StepPort != nullptr)
        *_StepPort &= static_cast<uint8_t>(~_StepMask);
    TCCR1B |= _BV(CS12);
}

void Motor::stopReferenceMotion()
{
    _StepRun = false;
    _StepLevel = false;
    if (_StepPort != nullptr)
        *_StepPort &= static_cast<uint8_t>(~_StepMask);
    Timer1_Stop();
}

void Motor::finishReference(bool valid)
{
    stopReferenceMotion();
    // Keep the queued job valid until the existing post-reference BG3 move
    // has completed; Motor::Update() owns final job cleanup.
    _ReferenceState = ReferenceState_t::IDLE;
    _JobResult = valid ? MotorJobResult_t::DONE : MotorJobResult_t::NONE;
    FY_Refrun = valid ? REF_STATE::REF_VALID : REF_STATE::REF_ERROR;
    _State = MotorState_t::REFRUN;
}

void Motor::updateReference()
{
    if (_ReferenceError)
    {
        finishReference(false);
        return;
    }

    switch (_ReferenceState)
    {
    case ReferenceState_t::REF_START:
        _ReferenceLeftEndHit = false;
        _ReferenceRightEndHit = false;
        _ReferenceLastSensorState = ((*_ReferenceInputPort & _ReferenceInputMask) != 0U);
        startReferenceMotion(false, V_MIN);
        _ReferenceState = ReferenceState_t::REF_LEFT_END;
        break;

    case ReferenceState_t::REF_LEFT_END:
        if (_ReferenceLeftEndHit)
        {
            stopReferenceMotion();
            if (!_ReferenceLongRun)
            {
                finishReference(true);
                break;
            }
            _ReferenceRightEndHit = false;
            startReferenceMotion(true,
                (V_MIN + 20U > V_MAX) ? V_MAX : static_cast<uint16_t>(V_MIN + 20U));
            _ReferenceState = ReferenceState_t::REF_SEARCH_RIGHT;
        }
        break;

    case ReferenceState_t::REF_SEARCH_RIGHT:
        if (_ReferenceRightEndHit)
        {
            stopReferenceMotion();
            if (!_ReferenceCoarseCaptured)
            {
                finishReference(false);
                break;
            }
            _ReferenceApproachTarget = static_cast<uint16_t>(
                _ReferenceCoarsePosition + REFERENCE_BACKOFF_STEPS);
            if (_ReferenceApproachTarget >= _ReferenceRightEndPosition)
                _ReferenceApproachTarget = (_ReferenceRightEndPosition > 100U)
                    ? static_cast<uint16_t>(_ReferenceRightEndPosition - 100U) : 0U;
            startReferenceMotion(false,
                (V_MIN + 20U > V_MAX) ? V_MAX : static_cast<uint16_t>(V_MIN + 20U));
            _ReferenceState = ReferenceState_t::REF_APPROACH_REF;
        }
        break;

    case ReferenceState_t::REF_APPROACH_REF:
        if (_Position <= _ReferenceApproachTarget)
        {
            stopReferenceMotion();
            _ReferenceLastSensorState = ((*_ReferenceInputPort & _ReferenceInputMask) != 0U);
            _ReferencePreciseRightCaptured = false;
            _ReferencePreciseCaptured = false;
            _ReferenceMeasurePass = 1U;
            _ReferenceMeasureArmed = true;
            _ReferenceStartValid = false;
            startReferenceMotion(false, V_MIN);
            _ReferenceState = ReferenceState_t::REF_RETURN_TO_REF;
        }
        break;

    case ReferenceState_t::REF_RETURN_TO_REF:
        if (_ReferencePreciseCaptured)
        {
            stopReferenceMotion();
            _ReferenceShiftTarget = (_ReferencePreciseLeft > REFERENCE_SHIFT_STEPS)
                ? static_cast<uint16_t>(_ReferencePreciseLeft - REFERENCE_SHIFT_STEPS) : 0U;
            startReferenceMotion(false, V_MIN);
            _ReferenceState = ReferenceState_t::REF_SHIFT_LEFT;
        }
        break;

    case ReferenceState_t::REF_SHIFT_LEFT:
        if (_Position <= _ReferenceShiftTarget)
        {
            stopReferenceMotion();
            _ReferenceLastSensorState = ((*_ReferenceInputPort & _ReferenceInputMask) != 0U);
            _ReferenceMeasurePass = 2U;
            _ReferenceMeasureArmed = true;
            _ReferenceSecondPreciseCaptured = false;
            _ReferenceStartValid = false;
            startReferenceMotion(true, V_MIN);
            _ReferenceState = ReferenceState_t::REF_MEASURE_LEFT;
        }
        break;

    case ReferenceState_t::REF_MEASURE_LEFT:
        if (_ReferenceSecondPreciseCaptured)
        {
            stopReferenceMotion();
            _ReferenceState = ReferenceState_t::REF_CALCULATE;
        }
        break;

    case ReferenceState_t::REF_CALCULATE:
    {
        _ReferencePlayLeft =
            static_cast<int16_t>(_ReferencePreciseLeft2) -
            static_cast<int16_t>(_ReferencePreciseLeft);
        _ReferencePlayRight =
            static_cast<int16_t>(_ReferencePreciseRight2) -
            static_cast<int16_t>(_ReferencePreciseRight);
        _ReferenceCenterShift =
            static_cast<int16_t>(_ReferencePreciseCenter2) -
            static_cast<int16_t>(_ReferencePreciseCenter);
        _ReferenceSpan = (_ReferenceRightEndPosition > REFERENCE_LEFT_POSITION)
            ? static_cast<uint16_t>(_ReferenceRightEndPosition - REFERENCE_LEFT_POSITION)
            : 0U;

        const bool valid = _ReferenceCoarseCaptured &&
            _ReferencePreciseCaptured && _ReferenceSecondPreciseCaptured &&
            (_ReferencePreciseLength > 0U) && (_ReferencePreciseLength2 > 0U) &&
            (_ReferenceSpan > 0U);
        finishReference(valid);
        break;
    }

    case ReferenceState_t::REF_ERROR:
    case ReferenceState_t::IDLE:
    default:
        finishReference(false);
        break;
    }
}


// -----------------------------------------------------------------------------
// Helping functions
// -----------------------------------------------------------------------------

bool Motor::prepareSetPosition()
{
    if (FY_Refrun != REF_STATE::REF_VALID)
        return false;

    // Protocol DATA is uint16 little-endian: DATA[0] = LSB.
    const uint16_t target = static_cast<uint16_t>(_Job.data[0]) |
        (static_cast<uint16_t>(_Job.data[1]) << 8U);

    if (target > POS_MAX ||
        ((_TotalLength > 0U) &&
         (target > static_cast<uint32_t>(1000U) + _TotalLength)))
        return false;

    _TargetPosition = target;
    move2Pos(static_cast<int16_t>(target));
    return (_StepRun || target == getPosition());
}

bool Motor::prepareLeft()
{
    if (FY_Refrun != REF_STATE::REF_VALID)
        return false;

    // LEFT selects the next measured track position to the left.
    static constexpr uint16_t trackPositions[5] =
        {1557U, 3157U, 4757U, 6357U, 7957U};
    const uint16_t current = getPosition();

    for (int8_t i = 4; i >= 0; --i)
    {
        if (trackPositions[static_cast<uint8_t>(i)] < current)
        {
            move2Pos(static_cast<int16_t>(trackPositions[static_cast<uint8_t>(i)]));
            return _StepRun || trackPositions[static_cast<uint8_t>(i)] == current;
        }
    }
    return false;
}

bool Motor::prepareRight()
{
    if (FY_Refrun != REF_STATE::REF_VALID)
        return false;

    // RIGHT selects the next measured track position to the right.
    static constexpr uint16_t trackPositions[5] =
        {1557U, 3157U, 4757U, 6357U, 7957U};
    const uint16_t current = getPosition();

    for (uint8_t i = 0; i < 5; ++i)
    {
        if (trackPositions[i] > current)
        {
            move2Pos(static_cast<int16_t>(trackPositions[i]));
            return _StepRun || trackPositions[i] == current;
        }
    }
    return false;
}

bool Motor::prepareSetTrack()
{
    if (FY_Refrun != REF_STATE::REF_VALID)
        return false;

    const uint8_t track = _Job.data[0];
    static constexpr uint16_t trackPositions[5] =
        {1557U, 3157U, 4757U, 6357U, 7957U};

    if (track < 1U || track > 5U)
        return false;

    const uint16_t target = trackPositions[track - 1U];
    if (target > POS_MAX)
        return false;

    move2Pos(static_cast<int16_t>(target));
    return _StepRun || target == getPosition();
}

bool Motor::CheckBorder()
{
    if (_TargetPosition > POS_MAX)
        return false;

    // If a measured span exists, the right limit is position 1000 + span.
    if ((_TotalLength > 0U) &&
        (_TargetPosition > static_cast<uint32_t>(1000U) + _TotalLength))
        return false;

    return true;
}

bool Motor::prepareParameter()
{
    // valider parameter? -> invalidparamError
    // parameter in range -> OORParamError
    return true;
}


// -----------------------------------------------------------------------------
// Status / information
// -----------------------------------------------------------------------------

bool Motor::isMoving()
{
    return (MotorState_t::MOVING == _State) ||
           (MotorState_t::REFRUN == _State);
}

bool Motor::isDriverActive() const
{
    return digitalRead(_ENA_pin) == LOW;
}

uint16_t Motor::getPosition()
{
    uint16_t position = 0U;

    // _Position is updated in the timer ISR; read both bytes atomically.
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        position = _Position;
    }

    return position;
}

uint16_t Motor::motor_getTargetPosition()
{
    return _TargetPosition;
}

MotorState_t Motor::motor_getState()
{
    return _State;
}

MotorJobResult_t Motor::getJobResult() const
{
    return _JobResult;
}


// -----------------------------------------------------------------------------
// Movement profile
// -----------------------------------------------------------------------------
// Die Profilberechnung bleibt in diesem Schritt unveraendert.
// Sie wird als eigener Schritt gegen die in FY_HW_TEST verifizierte
// Berechnung abgeglichen.

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


// -----------------------------------------------------------------------------
// Timer1
// -----------------------------------------------------------------------------

void Motor::Timer1_Init()
{
    noInterrupts();

    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;

    // CTC-Modus: TOP = OCR1A
    TCCR1B |= _BV(WGM12);

    // Compare-Match-A-Interrupt dauerhaft aktiv.
    // Die Bewegung wird separat ueber _StepRun freigegeben.
    TIMSK1 |= _BV(OCIE1A);

    OCR1A = TIMER_V_MIN;

    // FY_HW_TEST: Timer1 CTC mit Prescaler /256 (CS12=1, CS11=0, CS10=0).
    TCCR1B |= _BV(CS12);

    _StepRun = false;
    _StepLevel = false;
    _ProfileRampCounter = 0;

    interrupts();
}

void Motor::Timer1_Start()
{
    if (!_TimerValid)
        return;

    // Profil immer am Anfang beginnen.
    _ProfileElement = MotorProfile_t::ACC1;

    // Erstes nicht-leeres Profilelement suchen.
    _StepsRemaining = 0;

    for (uint8_t i = 0; i < 6; ++i)
    {
        const uint8_t element =
            static_cast<uint8_t>(_ProfileElement);

        _StepsRemaining = Motor_profil[element].Steps;

        if (_StepsRemaining > 0)
            break;

        switch (_ProfileElement)
        {
        case MotorProfile_t::ACC1:
            _ProfileElement = MotorProfile_t::ACC2;
            break;
        case MotorProfile_t::ACC2:
            _ProfileElement = MotorProfile_t::KONST;
            break;
        case MotorProfile_t::KONST:
            _ProfileElement = MotorProfile_t::BRE1;
            break;
        case MotorProfile_t::BRE1:
            _ProfileElement = MotorProfile_t::BRE2;
            break;
        case MotorProfile_t::BRE2:
            _ProfileElement = MotorProfile_t::POSI;
            break;
        case MotorProfile_t::POSI:
            break;
        }
    }

    _TimerValue = TIMER_V_MIN;
    _ProfileRampCounter = 0;
    _StepLevel = false;
    _StepRun = (_StepsRemaining > 0);

    OCR1A = _TimerValue;

    if (_StepRun)
    {
        // DIR must be stable before STEP pulses begin.
        if (_DirPort != nullptr)
        {
            if (_DirectionRight)
                *_DirPort |= _DirMask;
            else
                *_DirPort &= static_cast<uint8_t>(~_DirMask);
        }

        // Treiber aktivieren.
        digitalWrite(_ENA_pin, LOW);

        // Timer1 CTC: Prescaler /256, wie im aktuellen FY_HW_TEST.
        TCNT1 = 0;
        TCCR1B |= _BV(CS12);
    }
}

void Motor::Timer1_Stop()
{
    TCCR1B &= ~(_BV(CS12) | _BV(CS11) | _BV(CS10));
}

uint16_t Motor::current_profile_ramp_interval()
{
    switch (profileElement)
    {
    case static_cast<uint8_t>(MotorProfile_t::ACC1): return 1U;
    case static_cast<uint8_t>(MotorProfile_t::ACC2): return 2U;
    case static_cast<uint8_t>(MotorProfile_t::BRE1): return 2U;
    case static_cast<uint8_t>(MotorProfile_t::BRE2): return 1U;
    default: return 1U;
    }
}


// -----------------------------------------------------------------------------
// Timer1 ISR
// -----------------------------------------------------------------------------

void Motor::Timer1_ISR()
{
    const bool limitRight = ((*LSR.inputRegister) & LSR.mask) != 0;
    const bool reference  = ((*LSREF.inputRegister) & LSREF.mask) != 0;
    const bool limitLeft  = ((*LSL.inputRegister) & LSL.mask) != 0;

    if (!stepRun)
    {
        stepLevel = false;
        PORTD &= ~_BV(PD3);
        return;
    }

    // Endschalter haben Vorrang vor dem Profil.
    if (!io.dir && limitLeft)
    {
        stepRun = false;
        profileRun = false;
        stepLevel = false;
        io.ledLeft = false;
        updateOut = true;
        hw.displayDirty = true;
        PORTD &= ~_BV(PD3);
        return;
    }

    if (io.dir && limitRight)
    {
        stepRun = false;
        profileRun = false;
        stepLevel = false;
        io.ledRight = false;
        updateOut = true;
        hw.displayDirty = true;
        PORTD &= ~_BV(PD3);
        return;
    }

    stepLevel = !stepLevel;

    if (stepLevel)
    {
        PORTD |= _BV(PD3);
        return;
    }

    PORTD &= ~_BV(PD3);

    if (io.dir)
        measurement.position++;
    else if (measurement.position > 0)
        measurement.position--;

    if (!refLastState && reference)
    {
        measurement.refStart = measurement.position;
        refStartValid = true;
        measurement.refValid = false;

        if (io.dir && !refCoarseCaptured)
        {
            refCoarsePosition = measurement.position;
            refCoarseCaptured = true;
        }

        if (!io.dir && refMeasureArmed &&
            refMeasurePass == 1 && !refPreciseRightCaptured)
        {
            refPreciseRight = measurement.position;
            refPreciseRightCaptured = true;
        }

        if (io.dir && refMeasureArmed &&
            refMeasurePass == 2)
        {
            refPreciseLeft2 = measurement.position;
        }
    }
    else if (refLastState && !reference)
    {
        if (refStartValid)
        {
            measurement.refEnd = measurement.position;

            if (measurement.refEnd >= measurement.refStart)
                measurement.refLength =
                    measurement.refEnd - measurement.refStart;
            else
                measurement.refLength =
                    measurement.refStart - measurement.refEnd;

            measurement.refValid = true;
        }

        if (!io.dir && refMeasureArmed &&
            refMeasurePass == 1 && refPreciseRightCaptured)
        {
            refPreciseLeft = measurement.position;

            if (refPreciseRight >= refPreciseLeft)
                refPreciseLength = refPreciseRight - refPreciseLeft;
            else
                refPreciseLength = refPreciseLeft - refPreciseRight;

            refPreciseCenter =
                refPreciseLeft + (refPreciseLength / 2);

            refPreciseCaptured = true;
            refMeasureArmed = false;
        }

        if (io.dir && refMeasureArmed &&
            refMeasurePass == 2)
        {
            refPreciseRight2 = measurement.position;

            if (refPreciseRight2 >= refPreciseLeft2)
                refPreciseLength2 =
                    refPreciseRight2 - refPreciseLeft2;
            else
                refPreciseLength2 =
                    refPreciseLeft2 - refPreciseRight2;

            refPreciseCenter2 =
                refPreciseLeft2 + (refPreciseLength2 / 2);

            refSecondPreciseCaptured = true;
            refMeasureArmed = false;
        }
    }

    refLastState = reference;

    if (!profileRun)
        return;

    if (profileStepsRemaining > 0)
        --profileStepsRemaining;

    // Die aktuelle Profilphase bestimmt die Timeränderung.
    // Die Änderung erfolgt nur alle N STEP; dadurch wird die Rampe flacher.
    const int16_t rampAccel = motionProfile[profileElement].Accel;

    if (rampAccel != 0)
    {
        ++profileRampCounter;

        if (profileRampCounter >= current_profile_ramp_interval())
        {
            profileRampCounter = 0;

            profileTimerValue =
                static_cast<uint16_t>(
                    static_cast<int32_t>(profileTimerValue) + rampAccel);
        }
    }

    const uint16_t minReload = frequency_to_reload(profileVMax);
    const uint16_t maxReload = frequency_to_reload(profileVMin);

    if (profileTimerValue < minReload)
        profileTimerValue = minReload;
    if (profileTimerValue > maxReload)
        profileTimerValue = maxReload;

    OCR1A = profileTimerValue;

    if (profileTimerValue < profileMinReloadReached)
        profileMinReloadReached = profileTimerValue;

    if (profileStepsRemaining == 0)
    {
        ++profileElement;

        if (!load_next_profile_element())
        {
            profileRun = false;
            stepRun = false;
            profileReportPending = true;
            io.ledLeft = false;
            io.ledRight = false;
            updateOut = true;
            hw.displayDirty = true;
        }
    }
}


ISR(TIMER1_COMPA_vect)
{
    Motor::Timer1_ISR();
}
