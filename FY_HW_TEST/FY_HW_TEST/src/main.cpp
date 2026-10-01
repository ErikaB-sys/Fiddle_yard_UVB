#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <PCF8574.h>

#include "HW_Test.h"
#include "../../../FY_Common/MotorProfile.h"

Adafruit_SSD1306 display(128, 32, &Wire, -1);
PCF8574 expander(PORT_EXPANDER_ADDRESS);

HWIOState io;
HWTestState hw;

static uint32_t tButtons  = 0;
static uint32_t tSwitches = 0;
static uint32_t tDisplay  = 0;

static bool lastStop  = false;
static bool lastLeft  = false;
static bool lastRight = false;
static bool lastGo    = false;

static volatile bool updateOut = true;

// -----------------------------------------------------------------------------
// UART RX - menschenlesbares Testformat: CMD + Dezimalwert
// Beispiele: V800, v100, S500, A200, B300, R
// -----------------------------------------------------------------------------
static uint8_t serialCommand = 0;
static uint16_t serialValue = 0;
static bool serialDisplayDirty = false;

constexpr uint16_t V_MIN_LIMIT = 80;
constexpr uint16_t V_MAX_LIMIT = 1500;

static uint16_t profileVMin = V_MIN_LIMIT;
static uint16_t profileVMax = 800;

// Accel/Brake sind Timer-Ticks pro abgeschlossenem STEP.
// Kleinerer Timerwert = höhere Geschwindigkeit.
constexpr uint16_t ACCEL_MIN = 1;
constexpr uint16_t ACCEL_MAX = 10;

static uint16_t profileAccel1 = 2;
static uint16_t profileAccel2 = 2;
static uint16_t profileBrake1 = 2;
static uint16_t profileBrake2 = 2;
static uint16_t profileDistance = 1000;

// Rampenauflösung: Änderung des Timerwertes nur alle N Steps.
// 1 = Änderung bei jedem STEP, 2 = jedem zweiten STEP usw.
constexpr uint8_t RAMP_INTERVAL_MIN = 1;
constexpr uint8_t RAMP_INTERVAL_MAX = 10;

static uint8_t profileAccel1Interval = 1;

static volatile bool stepRun = false;
static volatile bool stepLevel = false;

volatile HWMeasurement measurement;
static uint8_t profileAccel2Interval = 1;
static uint8_t profileBrake1Interval = 1;
static uint8_t profileBrake2Interval = 1;

static volatile uint8_t profileRampCounter = 0;

// Gemeinsame Profilparameter: sechs Segmente, aber zunächst
// nur ein Beschleunigungs- und ein Bremswert.
constexpr uint16_t PROFILE_POSI_MIN = 160;
constexpr uint16_t PROFILE_KONST_MIN = 400;
constexpr uint16_t PROFILE_ACC1_STEPS = 100;
constexpr uint16_t PROFILE_ACC2_STEPS = 100;

static MotionProfileElementData_t motionProfile[6];
static volatile uint8_t profileElement = 0;
static volatile uint16_t profileStepsRemaining = 0;
static volatile uint16_t profileTimerValue = 0;
static volatile uint16_t profileMinReloadReached = 65535;
static volatile bool profileRun = false;
static volatile bool profileReportPending = false;

// -----------------------------------------------------------------------------
// Reference run - vollständiges State-Machine-Gerüst, zunächst nur Grundpfad.
// -----------------------------------------------------------------------------
enum class RefRunState : uint8_t
{
    IDLE,
    REF_START,
    REF_LEFT_END,
    REF_SEARCH_RIGHT,
    REF_RIGHT_END,
    REF_RETURN_TO_REF,
    REF_APPROACH_REF,
    REF_SLOW_REF,
    REF_MEASURE_RIGHT,
    REF_SHIFT_LEFT,
    REF_MEASURE_LEFT,
    REF_CALCULATE,
    REF_VALID,
    REF_ERROR
};

static RefRunState refRunState = RefRunState::IDLE;
static bool refRunActive = false;
static uint16_t refRunVMax = V_MIN_LIMIT;
static uint16_t refBackoffTarget = 0;

static volatile uint16_t refCoarsePosition = 0;
static volatile uint16_t refRightEndPosition = 0;
static volatile uint16_t refPreciseRight = 0;
static volatile uint16_t refPreciseLeft = 0;
static volatile uint16_t refPreciseLength = 0;
static volatile uint16_t refPreciseCenter = 0;
static volatile bool refCoarseCaptured = false;
static volatile bool refPreciseRightCaptured = false;
static volatile bool refPreciseCaptured = false;
static volatile bool refMeasureArmed = false;
static volatile bool refLastState = false;

// 20 mm Antriebsrad, 200 Schritte/Umdrehung, 1/8 Microstep:
// ca. 25.46 Schritte/mm -> 16 mm ca. 407 Schritte.
constexpr uint16_t REF_BACKOFF_STEPS = 407;

static void reference_run()
{
    if (!refRunActive)
        return;

    switch (refRunState)
    {
        case RefRunState::REF_START:
            refCoarseCaptured = false;
            refPreciseRightCaptured = false;
            refPreciseCaptured = false;
            refMeasureArmed = false;
            refCoarsePosition = 0;
            refRightEndPosition = 0;
            refPreciseRight = 0;
            refPreciseLeft = 0;
            refPreciseLength = 0;
            refPreciseCenter = 0;

            io.dir = false;
            updateOut = true;
            set_step_frequency(profileVMin);
            stepRun = true;

            Serial.println(F("REF: START"));
            refRunState = RefRunState::REF_LEFT_END;
            break;

        case RefRunState::REF_LEFT_END:
            if (io.limitLeft)
            {
                stepRun = false;
                profileRun = false;
                measurement.position = 1000;

                io.dir = true;
                updateOut = true;
                set_step_frequency(refRunVMax);
                stepRun = true;
                refRunState = RefRunState::REF_SEARCH_RIGHT;
            }
            break;

        case RefRunState::REF_SEARCH_RIGHT:
            if (io.limitRight)
            {
                stepRun = false;
                refRightEndPosition = measurement.position;

                noInterrupts();
                refLastState =
                    ((*LSREF.inputRegister) & LSREF.mask) != 0;
                refPreciseRightCaptured = false;
                refPreciseCaptured = false;
                refMeasureArmed = true;
                measurement.refValid = false;
                interrupts();

                io.dir = false;
                updateOut = true;
                set_step_frequency(V_MIN_LIMIT);
                stepRun = true;

                refRunState = RefRunState::REF_RETURN_TO_REF;
            }
            break;

        case RefRunState::REF_RETURN_TO_REF:
            if (refPreciseCaptured)
            {
                stepRun = false;
                profileRun = false;
                refRunState = RefRunState::REF_CALCULATE;
            }
            break;

        case RefRunState::REF_CALCULATE:
            {
                const int16_t coarseError =
                    static_cast<int16_t>(refCoarsePosition) -
                    static_cast<int16_t>(refPreciseCenter);

                Serial.print(F("REF: DATA COARSE="));
                Serial.print(refCoarsePosition);
                Serial.print(F(" RIGHT="));
                Serial.print(refPreciseRight);
                Serial.print(F(" LEFT="));
                Serial.print(refPreciseLeft);
                Serial.print(F(" LEN="));
                Serial.print(refPreciseLength);
                Serial.print(F(" CENTER="));
                Serial.print(refPreciseCenter);
                Serial.print(F(" COARSE_ERR="));
                Serial.print(coarseError);
                Serial.print(F(" LSR="));
                Serial.print(refRightEndPosition);
                Serial.print(F(" SPAN="));
                Serial.println(
                    refRightEndPosition > 1000
                        ? refRightEndPosition - 1000
                        : 0);

                refBackoffTarget = refPreciseCenter;
                io.dir = true;
                updateOut = true;
                set_step_frequency(V_MIN_LIMIT);
                stepRun = true;
                refRunState = RefRunState::REF_VALID;
            }
            break;

        case RefRunState::REF_VALID:
            if (measurement.position >= refBackoffTarget)
            {
                stepRun = false;
                profileRun = false;
                refRunActive = false;
                hw.displayDirty = true;
                Serial.println(F("REF: VALID"));
                refRunState = RefRunState::IDLE;
            }
            break;

        case RefRunState::REF_ERROR:
            stepRun = false;
            profileRun = false;
            refRunActive = false;
            hw.displayDirty = true;
            Serial.println(F("REF: ERROR"));
            refRunState = RefRunState::IDLE;
            break;

        case RefRunState::REF_APPROACH_REF:
        case RefRunState::REF_SLOW_REF:
        case RefRunState::REF_MEASURE_RIGHT:
        case RefRunState::REF_SHIFT_LEFT:
        case RefRunState::REF_MEASURE_LEFT:
        case RefRunState::IDLE:
        default:
            refRunActive = false;
            refRunState = RefRunState::IDLE;
            break;
    }
}
void update_outputs()
{
    digitalWrite(MOTOR_ENA, io.ena ? HIGH : LOW);
    digitalWrite(MOTOR_DIR, io.dir ? HIGH : LOW);

    expander.digitalWrite(LED_STOP_PIN, io.ledStop ? LOW : HIGH);
    expander.digitalWrite(LED_GO_PIN, io.ledGo ? LOW : HIGH);
    expander.digitalWrite(LED_LEFT_PIN, io.ledLeft ? LOW : HIGH);
    expander.digitalWrite(LED_RIGHT_PIN, io.ledRight ? LOW : HIGH);
}

static volatile bool refLastState = false;
static volatile bool refStartValid = false;
static volatile bool leftReferenceApplied = false;
static volatile bool rightLengthCaptured = false;

ISR(TIMER1_COMPA_vect)
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

        if (!io.dir && refMeasureArmed && refPreciseRightCaptured)
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

        if (!io.dir && refMeasureArmed && !refPreciseRightCaptured)
        {
            refPreciseRight = measurement.position;
            refPreciseRightCaptured = true;
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

void set_step_frequency(uint16_t frequency)
{
    if (frequency == 0)
        return;

    uint32_t denominator = 2UL * frequency;
    uint32_t reload = ((F_CPU / 256UL) + (denominator / 2UL)) / denominator - 1UL;

    if (reload > 65535UL)
        reload = 65535UL;

    if (reload < 1UL)
        reload = 1UL;

    noInterrupts();
    OCR1A = static_cast<uint16_t>(reload);
    interrupts();
}

void init_step_timer()
{
    noInterrupts();
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;
    TCCR1B |= _BV(WGM12);
    TCCR1B |= _BV(CS12);             // Timer1 Prescaler /256 (CS12=1, CS11=0, CS10=0)
    TIMSK1 |= _BV(OCIE1A);
    stepRun = false;
    stepLevel = false;
    profileRun = false;
    profileElement = 0;
    profileStepsRemaining = 0;
    profileMinReloadReached = 65535;
    profileReportPending = false;
    measurement.position = 1000;
    measurement.refStart = 0;
    measurement.refEnd = 0;
    measurement.refLength = 0;
    measurement.totalLength = MAX_ALLOWED_STEPS;
    measurement.refValid = false;
    refStartValid = false;
    leftReferenceApplied = false;
    rightLengthCaptured = false;
    refLastState = (PINB & _BV(PB3)) != 0;
    refCoarseCaptured = false;
    refPreciseRightCaptured = false;
    refPreciseCaptured = false;
    refMeasureArmed = false;
    PORTD &= ~_BV(PD3);
    interrupts();

    set_step_frequency(STEP_FREQUENCIES[0]);
}

void get_measurement(HWMeasurement& result)
{
    noInterrupts();
    result.position = measurement.position;
    result.refStart = measurement.refStart;
    result.refEnd = measurement.refEnd;
    result.refLength = measurement.refLength;
    result.totalLength = measurement.totalLength;
    result.refValid = measurement.refValid;
    interrupts();
}

void update_display()
{
    display.clearDisplay();
    display.setCursor(0, 0);

    static HWMeasurement displayMeasurement;

    get_measurement(displayMeasurement);

    const HWMeasurement& m = displayMeasurement;

    // Zeile 1: Motorstatus und gültiger Geschwindigkeitsbereich.
    display.print(F("M: "));
    display.print(io.ena ? F("ON") : F("OFF"));
    display.print(F(" V: "));
    display.print(profileVMin);
    display.print(F(" | "));
    display.println(profileVMax);

    // Zeile 2: aktuelles Profilsegment / Position / Track.
    display.print(F("S: "));
    display.print(profileElement);
    display.print(F(" Pos: "));
    display.print(m.position);
    display.println(F(" TRK: -"));

    // Zeile 3: Endschalter und Referenzsensor.
    display.print(F("LS:L"));
    display.print(io.limitLeft ? '1' : '0');
    display.print(F(" R"));
    display.print(io.limitRight ? '1' : '0');
    display.print(F(" REF"));
    display.print(io.reference ? '1' : '0');
    display.print(F(" H"));
    display.println(io.hall ? '1' : '0');

    // Zeile 4: interner Testzustand.
    display.print(F("State: "));
    if (refRunActive)
    {
        switch (refRunState)
        {
            case RefRunState::REF_START:       display.println(F("REF_START")); break;
            case RefRunState::REF_LEFT_END:   display.println(F("REF_LEFT")); break;
            case RefRunState::REF_SEARCH_RIGHT: display.println(F("REF_RIGHT")); break;
            case RefRunState::REF_RIGHT_END:  display.println(F("REF_END")); break;
            case RefRunState::REF_VALID:      display.println(F("REF_VALID")); break;
            default:                           display.println(F("REF")); break;
        }
    }
    else if (hw.activeButton == BUTTON_STOP)
        display.println(F("STOP"));
    else if (profileRun)
        display.println(F("MOVE"));
    else
        display.println(F("IDLE"));

    display.display();
}

void setup()
{
    Serial.begin(115200);

    Wire.begin();
    delay(100);

    scanI2C();
    initOLED();
    init_expander();

    pinMode(LSL.pin, INPUT);
    pinMode(LSR.pin, INPUT);
    pinMode(LSREF.pin, INPUT);
    pinMode(LS4.pin, INPUT);

    pinMode(MOTOR_ENA, OUTPUT);
    pinMode(MOTOR_PULS, OUTPUT);
    pinMode(MOTOR_DIR, OUTPUT);
    pinMode(LED_BUILTIN_PIN, OUTPUT);

    init_step_timer();

    hw.ena  = false;
    hw.dir  = false;
    io.ena  = false;
    io.dir  = false;
    hw.puls = false;
    stepRun = false;
    profileRun = false;
    hw.stepReload = 100;
    hw.frequency = STEP_FREQUENCIES[0];
    hw.steps = profileDistance;

    clear_active_button();

    clamp_profile();
    Serial.println(F("init done"));
}

void loop()
{
    const uint32_t now = millis();

    update_serial();
    reference_run();

    if (profileReportPending)
    {
        uint16_t reachedReload;

        noInterrupts();
        reachedReload = profileMinReloadReached;
        profileReportPending = false;
        interrupts();

        Serial.print(F("VMAX_REACHED: "));
        Serial.print(reload_to_frequency(reachedReload));
        Serial.println(F(" steps/s"));
    }

    if (now - tButtons >= BUTTON_INTERVAL_MS)
    {
        tButtons = now;

        read_IO();
        const ButtonEvent event = read_buttons();

        if (event != BUTTON_NONE)
        {
            if (event == BUTTON_STOP || (!hw.busy && !profileRun))
            {
                hw.displayDirty = true;

                switch (event)
                {
                    case BUTTON_STOP:
                        hw.ena = false;
                        io.ena = false;
                        stepRun = false;
                        profileRun = false;
                        refRunActive = false;
                        refRunState = RefRunState::IDLE;
                        set_active_button(BUTTON_STOP);
                        break;

                    case BUTTON_GO:
                        hw.ena = true;
                        io.ena = true;
                        stepRun = false;
                        set_step_frequency(hw.frequency);
                        set_active_button(BUTTON_GO);
                        break;

                    case BUTTON_LEFT:
                        // Endschalterbegrenzung beim Start bleibt erhalten.
                        if ((true ==io.limitLeft) && (true== io.ena ))
                            break;

                        if (!io.ena)
                        {
                            for (uint8_t i = 0; i < STEP_FREQUENCY_COUNT; ++i)
                            {
                                if (STEP_FREQUENCIES[i] == hw.frequency && i > 0)
                                {
                                    hw.frequency = STEP_FREQUENCIES[i - 1];
                                    set_step_frequency(hw.frequency);
                                    set_active_button(BUTTON_LEFT);
                                    break;
                                }
                            }
                        }
                        else
                        {
                            hw.dir = false;
                            io.dir = false;

                            clamp_profile();
                            load_motion_profile();

                            profileElement = 0;
                            profileRampCounter = 0;
                            profileTimerValue = frequency_to_reload(profileVMin);
                            profileMinReloadReached = profileTimerValue;
                            profileReportPending = false;
                            profileMinReloadReached = profileTimerValue;
                            profileReportPending = false;
                            load_next_profile_element();
                            profileRun = true;
                            stepRun = true;

                            Serial.print(F("MOVE: DIST="));
                            Serial.print(profileDistance);
                            Serial.print(F(" A1="));
                            Serial.print(profileAccel1);
                            Serial.print(F(" A2="));
                            Serial.print(profileAccel2);
                            Serial.print(F(" A1N="));
                            Serial.print(profileAccel1Interval);
                            Serial.print(F(" A2N="));
                            Serial.print(profileAccel2Interval);
                            Serial.print(F(" VMIN="));
                            Serial.print(profileVMin);
                            Serial.print(F(" VMAX="));
                            Serial.print(profileVMax);
                            Serial.print(F(" B1="));
                            Serial.print(profileBrake1);
                            Serial.print(F(" B2="));
                            Serial.print(profileBrake2);
                            Serial.print(F(" B1N="));
                            Serial.print(profileBrake1Interval);
                            Serial.print(F(" B2N="));
                            Serial.println(profileBrake2Interval);

                            Serial.print(F("PROFILE: "));
                            for (uint8_t i = 0; i < 6; ++i)
                            {
                                if (i > 0)
                                    Serial.print(',');
                                Serial.print(motionProfile[i].Steps);
                            }
                            Serial.println();

                            set_active_button(BUTTON_LEFT);
                        }
                        break;

                    case BUTTON_RIGHT:
                        // Endschalterbegrenzung beim Start bleibt erhalten.
                        if ((true == io.limitRight)&& (true== io.ena))
                            break;

                        if (!io.ena)
                        {
                            for (uint8_t i = 0; i < STEP_FREQUENCY_COUNT; ++i)
                            {
                                if (STEP_FREQUENCIES[i] == hw.frequency && i + 1 < STEP_FREQUENCY_COUNT)
                                {
                                    hw.frequency = STEP_FREQUENCIES[i + 1];
                                    set_step_frequency(hw.frequency);
                                    set_active_button(BUTTON_RIGHT);
                                    break;
                                }
                            }
                        }
                        else
                        {
                            hw.dir = true;
                            io.dir = true;

                            clamp_profile();
                            load_motion_profile();

                            profileElement = 0;
                            profileRampCounter = 0;
                            profileTimerValue = frequency_to_reload(profileVMin);
                            profileMinReloadReached = profileTimerValue;
                            profileReportPending = false;
                            load_next_profile_element();
                            profileRun = true;
                            stepRun = true;

                            Serial.print(F("MOVE: DIST="));
                            Serial.print(profileDistance);
                            Serial.print(F(" A1="));
                            Serial.print(profileAccel1);
                            Serial.print(F(" A2="));
                            Serial.print(profileAccel2);
                            Serial.print(F(" A1N="));
                            Serial.print(profileAccel1Interval);
                            Serial.print(F(" A2N="));
                            Serial.print(profileAccel2Interval);
                            Serial.print(F(" VMIN="));
                            Serial.print(profileVMin);
                            Serial.print(F(" VMAX="));
                            Serial.print(profileVMax);
                            Serial.print(F(" B1="));
                            Serial.print(profileBrake1);
                            Serial.print(F(" B2="));
                            Serial.print(profileBrake2);
                            Serial.print(F(" B1N="));
                            Serial.print(profileBrake1Interval);
                            Serial.print(F(" B2N="));
                            Serial.println(profileBrake2Interval);
                            Serial.print(F("PROFILE: "));
                            for (uint8_t i = 0; i < 6; ++i)
                            {
                                if (i > 0)
                                    Serial.print(',');
                                Serial.print(motionProfile[i].Steps);
                            }
                            Serial.println();

                            set_active_button(BUTTON_RIGHT);
                        }
                        break;

                    default:
                        break;
                }
            }
        }
    }

    // Nur die aktive Bewegung wird durch die LS beendet.
    // Das LED-/Button-Verhalten bleibt wie beim normalen Loslassen.
    if (now - tSwitches >= SWITCH_INTERVAL_MS)
    {
        tSwitches = now;
        read_end_switches();
    }

    if (hw.busy && !active_button_pressed())
    {
        clear_active_button();
    }

    if (now - tDisplay >= DISPLAY_INTERVAL_MS)
    {
        tDisplay = now;

        // D13 dient als Loop-Heartbeat. Der Pin wird nicht mehr in der ISR benutzt.
        digitalWrite(LED_BUILTIN_PIN, !digitalRead(LED_BUILTIN_PIN));

        // LS/REF/Hall sollen auch während/nach der Fahrt live sichtbar sein.
        update_display();
        hw.displayDirty = false;
        serialDisplayDirty = false;
    }

    if (updateOut)
    {
        noInterrupts();
        updateOut = false;
        interrupts();

        update_outputs();
    }
}
