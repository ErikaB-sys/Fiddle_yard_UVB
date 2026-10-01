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

// -----------------------------------------------------------------------------
// Gleispositionen - Messwerte aus der aktuellen mechanischen Vermessung.
// 0 = Position noch nicht vermessen.
// Die Position bezeichnet die Gleismitte der Brücke.
// -----------------------------------------------------------------------------
constexpr uint8_t TRACK_COUNT = 5;
constexpr uint16_t TRACK_POSITION_UNKNOWN = 0;

static const uint16_t trackPosition[TRACK_COUNT] =
{
    1557,   // BG1
    3157,   // BG2
    4757,   // BG3
    6357,   // BG4
    7957    // BG5
};

static uint8_t selectedTrack = 0;
static bool trackMode = false;

// Lokale Bedienung: vor dem Start wird zwischen TRACK und LONG REF gewählt.
// Nach dem Start dienen die blauen Tasten zur Gleisauswahl.
enum class LocalStartMode : uint8_t
{
    TRACK,
    LONG_REF
};

static LocalStartMode localStartMode = LocalStartMode::TRACK;
static bool localSequenceActive = false;
static bool localTrackActive = false;

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
static bool refLongRun = false;
static uint16_t refRunVMax = V_MIN_LIMIT;
static uint16_t refBackoffTarget = 0;

static volatile uint16_t refCoarsePosition = 0;
static volatile uint16_t refRightEndPosition = 0;
static volatile uint16_t refPreciseRight = 0;
static volatile uint16_t refPreciseLeft = 0;
static volatile uint16_t refPreciseLength = 0;
static volatile uint16_t refPreciseCenter = 0;
static volatile uint16_t refPreciseRight2 = 0;
static volatile uint16_t refPreciseLeft2 = 0;
static volatile uint16_t refPreciseLength2 = 0;
static volatile uint16_t refPreciseCenter2 = 0;
static volatile uint16_t refApproachTarget = 0;
static volatile uint16_t refShiftTarget = 0;
static volatile uint8_t refMeasurePass = 0;
static volatile bool refCoarseCaptured = false;
static volatile bool refPreciseRightCaptured = false;
static volatile bool refPreciseCaptured = false;
static volatile bool refSecondPreciseCaptured = false;
static volatile bool refMeasureArmed = false;

static volatile bool refLastState = false;

static void reference_run();
static bool start_position_move(uint16_t targetPosition);


static uint16_t clamp_accel_brake(uint16_t value)
{
    if (value < ACCEL_MIN)
        return ACCEL_MIN;
    if (value > ACCEL_MAX)
        return ACCEL_MAX;
    return value;
}

static uint8_t clamp_ramp_interval(uint16_t value)
{
    if (value < RAMP_INTERVAL_MIN)
        return RAMP_INTERVAL_MIN;
    if (value > RAMP_INTERVAL_MAX)
        return RAMP_INTERVAL_MAX;
    return static_cast<uint8_t>(value);
}

static void clamp_profile()
{
    if (profileVMin < V_MIN_LIMIT)
        profileVMin = V_MIN_LIMIT;
    if (profileVMin > V_MAX_LIMIT)
        profileVMin = V_MAX_LIMIT;

    if (profileVMax < V_MIN_LIMIT)
        profileVMax = V_MIN_LIMIT;
    if (profileVMax > V_MAX_LIMIT)
        profileVMax = V_MAX_LIMIT;

    if (profileVMin > profileVMax)
        profileVMin = profileVMax;

    profileAccel1 = clamp_accel_brake(profileAccel1);
    profileAccel2 = clamp_accel_brake(profileAccel2);
    profileBrake1 = clamp_accel_brake(profileBrake1);
    profileBrake2 = clamp_accel_brake(profileBrake2);

    profileAccel1Interval = clamp_ramp_interval(profileAccel1Interval);
    profileAccel2Interval = clamp_ramp_interval(profileAccel2Interval);
    profileBrake1Interval = clamp_ramp_interval(profileBrake1Interval);
    profileBrake2Interval = clamp_ramp_interval(profileBrake2Interval);

    if (profileDistance == 0)
        profileDistance = 1;
    if (profileDistance > MAX_ALLOWED_STEPS)
        profileDistance = MAX_ALLOWED_STEPS;
}

static uint16_t reload_to_frequency(uint16_t reload)
{
    if (reload == 0)
        return 0;

    const uint32_t timerClock = F_CPU / 256UL;
    const uint32_t frequency =
        (timerClock + reload) / (2UL * (static_cast<uint32_t>(reload) + 1UL));

    return static_cast<uint16_t>(frequency);
}

static uint16_t frequency_to_reload(uint16_t frequency)
{
    uint32_t denominator = 2UL * frequency;
    uint32_t reload =
        ((F_CPU / 256UL) + (denominator / 2UL)) / denominator - 1UL;

    if (reload > 65535UL)
        reload = 65535UL;

    if (reload < 1UL)
        reload = 1UL;

    return static_cast<uint16_t>(reload);
}

static void load_motion_profile()
{
    MotionProfileParam_t param =
    {
        PROFILE_POSI_MIN,
        PROFILE_KONST_MIN,
        PROFILE_ACC1_STEPS,
        PROFILE_ACC2_STEPS,
        -static_cast<int16_t>(profileAccel1),
        -static_cast<int16_t>(profileAccel2),
        static_cast<int16_t>(profileBrake1),
        static_cast<int16_t>(profileBrake2)
    };

    calculateMotionProfile(profileDistance, param, motionProfile);
}

static uint8_t current_profile_ramp_interval()
{
    switch (profileElement)
    {
        case static_cast<uint8_t>(MotionProfileElement_t::ACC1):
            return profileAccel1Interval;

        case static_cast<uint8_t>(MotionProfileElement_t::ACC2):
            return profileAccel2Interval;

        case static_cast<uint8_t>(MotionProfileElement_t::BRE1):
            return profileBrake1Interval;

        case static_cast<uint8_t>(MotionProfileElement_t::BRE2):
            return profileBrake2Interval;

        default:
            return 1;
    }
}

static bool load_next_profile_element()
{

    while (profileElement < 6)
    {
        profileStepsRemaining = motionProfile[profileElement].Steps;

        if (profileStepsRemaining > 0)
        {
            profileRampCounter = 0;
            OCR1A = profileTimerValue;
            return true;
        }

        ++profileElement;
    }

    return false;
}

static bool start_position_move(uint16_t targetPosition)
{
    if (!io.ena)
    {
        Serial.println(F("MOTOR OFF!"));
        return false;
    }

    uint16_t currentPosition;

    noInterrupts();
    currentPosition = measurement.position;
    interrupts();

    if (targetPosition == currentPosition)
    {
        profileRun = false;
        stepRun = false;
        io.ledLeft = false;
        io.ledRight = false;
        updateOut = true;
        Serial.print(F("MOVE: AT POS="));
        Serial.println(targetPosition);
        return true;
    }

    if (targetPosition > currentPosition)
    {
        hw.dir = true;
        io.dir = true;
        io.ledRight = true;
        io.ledLeft = false;
    }
    else
    {
        hw.dir = false;
        io.dir = false;
        io.ledRight = false;
        io.ledLeft = true;
    }

    profileDistance = (targetPosition > currentPosition)
        ? (targetPosition - currentPosition)
        : (currentPosition - targetPosition);

    clamp_profile();
    load_motion_profile();

    profileElement = 0;
    profileRampCounter = 0;
    profileTimerValue = frequency_to_reload(profileVMin);
    profileMinReloadReached = profileTimerValue;
    profileReportPending = false;

    if (!load_next_profile_element())
        return false;

    profileRun = true;
    stepRun = true;

    hw.activeButton = BUTTON_NONE;
    hw.busy = false;
    hw.button = BUTTON_NONE;
    updateOut = true;
    hw.displayDirty = true;

    Serial.print(F("MOVE: POS="));
    Serial.print(currentPosition);
    Serial.print(F("->"));
    Serial.print(targetPosition);
    Serial.print(F(" DIST="));
    Serial.println(profileDistance);

    return true;
}

static void serial_process_line(const char* line)
{
    if (line[0] == '\0')
        return;

    serialCommand = static_cast<uint8_t>(line[0]);
    serialValue = 0;

    if (line[1] != '\0')
        serialValue = static_cast<uint16_t>(strtoul(&line[1], nullptr, 10));

    // A1N/A2N/B1N/B2N setzen die Rampenauflösung unabhängig.
    // Beispiel: A1N3 = A1 nur jeden dritten STEP anwenden.
    if (line[0] == 'A' && (line[1] == '1' || line[1] == '2') && line[2] == 'N')
    {
        const uint8_t value = clamp_ramp_interval(
            static_cast<uint16_t>(strtoul(&line[3], nullptr, 10)));

        if (line[1] == '1')
            profileAccel1Interval = value;
        else
            profileAccel2Interval = value;

        serialDisplayDirty = true;
        Serial.print(F("RX: A"));
        Serial.print(line[1]);
        Serial.print(F("N="));
        Serial.println(value);
        return;
    }

    if (line[0] == 'B' && (line[1] == '1' || line[1] == '2') && line[2] == 'N')
    {
        const uint8_t value = clamp_ramp_interval(
            static_cast<uint16_t>(strtoul(&line[3], nullptr, 10)));

        if (line[1] == '1')
            profileBrake1Interval = value;
        else
            profileBrake2Interval = value;

        serialDisplayDirty = true;
        Serial.print(F("RX: B"));
        Serial.print(line[1]);
        Serial.print(F("N="));
        Serial.println(value);
        return;
    }

    // A1/B1 setzen die jeweilige Stufe und synchronisieren die zweite Stufe.
    // A2/B2 überschreiben dagegen ausschließlich die zweite Stufe.
    if (line[0] == 'A' && line[1] == '1')
    {
        profileAccel1 = clamp_accel_brake(
            static_cast<uint16_t>(strtoul(&line[2], nullptr, 10)));
        profileAccel2 = profileAccel1;
        serialDisplayDirty = true;
        Serial.print(F("RX: A1/A2="));
        Serial.println(profileAccel1);
        return;
    }

    if (line[0] == 'A' && line[1] == '2')
    {
        profileAccel2 = clamp_accel_brake(
            static_cast<uint16_t>(strtoul(&line[2], nullptr, 10)));
        serialDisplayDirty = true;
        Serial.print(F("RX: A2="));
        Serial.println(profileAccel2);
        return;
    }

    if (line[0] == 'B' && line[1] == '1')
    {
        profileBrake1 = clamp_accel_brake(
            static_cast<uint16_t>(strtoul(&line[2], nullptr, 10)));
        profileBrake2 = profileBrake1;
        serialDisplayDirty = true;
        Serial.print(F("RX: B1/B2="));
        Serial.println(profileBrake1);
        return;
    }

    if (line[0] == 'B' && line[1] == '2')
    {
        profileBrake2 = clamp_accel_brake(
            static_cast<uint16_t>(strtoul(&line[2], nullptr, 10)));
        serialDisplayDirty = true;
        Serial.print(F("RX: B2="));
        Serial.println(profileBrake2);
        return;
    }

    switch (serialCommand)
    {
        case 'T':
        {
            const uint16_t track = serialValue;

            if (track == 0)
            {
                trackMode = false;
                selectedTrack = 0;
                serialDisplayDirty = true;
                Serial.println(F("RX: TRACK MODE OFF"));
                break;
            }

            if (track > TRACK_COUNT)
            {
                Serial.println(F("RX: TRACK INVALID"));
                break;
            }

            if (trackPosition[track - 1] == TRACK_POSITION_UNKNOWN)
            {
                Serial.print(F("RX: BG"));
                Serial.print(track);
                Serial.println(F(" POSITION UNKNOWN"));
                break;
            }

            selectedTrack = static_cast<uint8_t>(track);
            trackMode = true;
            serialDisplayDirty = true;

            Serial.print(F("RX: BG"));
            Serial.print(track);
            Serial.print(F(" POS="));
            Serial.println(trackPosition[track - 1]);

            start_position_move(trackPosition[track - 1]);
            break;
        }

        case 'P':
        {
            const uint16_t targetPosition = serialValue;

            if (targetPosition > MAX_ALLOWED_STEPS)
            {
                Serial.println(F("RX: POSITION INVALID"));
                break;
            }

            trackMode = false;
            selectedTrack = 0;
            serialDisplayDirty = true;

            Serial.print(F("RX: POS="));
            Serial.println(targetPosition);

            start_position_move(targetPosition);
            break;
        }

        case 'R':
        case 'r':
            if (!io.ena)
            {
                Serial.println(F("MOTOR OFF!"));
                break;
            }

            if (!refRunActive)
            {
                refRunState = RefRunState::REF_START;
                refLongRun = false;
                refRunActive = true;
                serialDisplayDirty = true;
                Serial.println(F("RX: REF_SHORT"));
            }
            break;

        case 'K':
        case 'k':
            if (!io.ena)
            {
                Serial.println(F("MOTOR OFF!"));
                break;
            }

            if (!refRunActive)
            {
                refRunState = RefRunState::REF_START;
                refLongRun = true;
                refRunActive = true;
                serialDisplayDirty = true;
                Serial.println(F("RX: REF_LONG"));
            }
            break;

        case 'V':
            profileVMax = serialValue;
            clamp_profile();
            if (hw.frequency > profileVMax)
                hw.frequency = profileVMax;
            set_step_frequency(hw.frequency);
            serialDisplayDirty = true;
            Serial.print(F("RX: V"));
            Serial.println(profileVMax);
            break;

        case 'v':
            profileVMin = serialValue;
            clamp_profile();
            if (hw.frequency < profileVMin)
                hw.frequency = profileVMin;
            set_step_frequency(hw.frequency);
            serialDisplayDirty = true;
            Serial.print(F("RX: v"));
            Serial.println(profileVMin);
            break;

        case 'A':
            // Rückwärtskompatibel: A setzt A1 und A2 synchron.
            profileAccel1 = clamp_accel_brake(serialValue);
            profileAccel2 = profileAccel1;
            serialDisplayDirty = true;
            Serial.print(F("RX: A1/A2="));
            Serial.println(profileAccel1);
            break;

        case 'B':
            // Rückwärtskompatibel: B setzt B1 und B2 synchron.
            profileBrake1 = clamp_accel_brake(serialValue);
            profileBrake2 = profileBrake1;
            serialDisplayDirty = true;
            Serial.print(F("RX: B1/B2="));
            Serial.println(profileBrake1);
            break;

        case 'S':
            profileDistance = serialValue;
            clamp_profile();
            serialDisplayDirty = true;
            Serial.print(F("RX: S"));
            Serial.println(profileDistance);
            break;

        default:
            Serial.print(F("RX: unknown command "));
            Serial.println(static_cast<char>(serialCommand));
            break;
    }
}

void update_serial()
{
    static char serialLine[12];
    static uint8_t serialLineLength = 0;

    while (Serial.available() > 0)
    {
        const char data = static_cast<char>(Serial.read());

        if (data == '\r' || data == '\n')
        {
            serialLine[serialLineLength] = '\0';
            serial_process_line(serialLine);
            serialLineLength = 0;
        }
        else if (serialLineLength < sizeof(serialLine) - 1)
        {
            serialLine[serialLineLength++] = data;
        }
        else
        {
            serialLineLength = 0;
            Serial.println(F("RX: input too long"));
        }
    }
}

void scanI2C()
{
    Serial.println(F("I2C Scanner"));
    uint8_t found = 0;

    for (uint8_t address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);

        if (Wire.endTransmission() == 0)
        {
            Serial.print(F("I2C device: 0x"));

            if (address < 0x10)
                Serial.print('0');

            Serial.println(address, HEX);
            found++;
        }
    }

    if (found == 0)
        Serial.println(F("No I2C devices found"));

    Serial.print(F("I2C devices found: "));
    Serial.println(found);
}

void initOLED()
{
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println(F("Error initializing OLED!"));
        while (true) {}
    }

    display.setRotation(2);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    display.println(F("FY HW TEST"));
    display.println(F("OLED: OK"));
    display.println(F("I2C: 0x3C"));

    display.display();
}

// static void all_mapping_leds_off();

void init_expander()
{
    expander.pinMode(BUTTON_STOP_PIN, INPUT);
    expander.pinMode(BUTTON_GO_PIN, INPUT);
    expander.pinMode(BUTTON_LEFT_PIN, INPUT);
    expander.pinMode(BUTTON_RIGHT_PIN, INPUT);

    expander.pinMode(LED_STOP_PIN, OUTPUT);
    expander.pinMode(LED_GO_PIN, OUTPUT);
    expander.pinMode(LED_LEFT_PIN, OUTPUT);
    expander.pinMode(LED_RIGHT_PIN, OUTPUT);

    if (!expander.begin())
    {
        Serial.println(F("Error initializing PCF8574!"));
        while (true) {}
    }
}

void read_IO()
{
    io.buttonStop  = (expander.digitalRead(BUTTON_STOP_PIN)  == LOW);
    io.buttonLeft  = (expander.digitalRead(BUTTON_LEFT_PIN)  == LOW);
    io.buttonRight = (expander.digitalRead(BUTTON_RIGHT_PIN) == LOW);
    io.buttonGo    = (expander.digitalRead(BUTTON_GO_PIN)    == LOW);

    io.limitLeft   = digitalRead(LSL.pin);
    io.limitRight  = digitalRead(LSR.pin);
    io.reference   = digitalRead(LSREF.pin);
    io.hall        = digitalRead(LS4.pin);
}

ButtonEvent read_buttons()
{
    const bool stop  = io.buttonStop;
    const bool left  = io.buttonLeft;
    const bool right = io.buttonRight;
    const bool go    = io.buttonGo;

    ButtonEvent event = BUTTON_NONE;

    if (stop && !lastStop)
        event = BUTTON_STOP;
    else if (left && !lastLeft)
        event = BUTTON_LEFT;
    else if (right && !lastRight)
        event = BUTTON_RIGHT;
    else if (go && !lastGo)
        event = BUTTON_GO;

    lastStop  = stop;
    lastLeft  = left;
    lastRight = right;
    lastGo    = go;

    return event;
}

void set_active_button(ButtonEvent event)
{
    hw.activeButton = event;
    hw.busy = (event != BUTTON_NONE);
    hw.button = event;

    io.ledStop  = !io.ena;
    io.ledGo    = io.ena;
    io.ledLeft  = (event == BUTTON_LEFT);
    io.ledRight = (event == BUTTON_RIGHT);

    updateOut = true;
    hw.displayDirty = true;
}

void clear_active_button()
{
    hw.activeButton = BUTTON_NONE;
    hw.busy = false;
    hw.button = BUTTON_NONE;

    io.ledStop = !io.ena;
    io.ledGo = io.ena;
    io.ledLeft = false;
    io.ledRight = false;

    updateOut = true;
    hw.displayDirty = true;
}

static bool active_button_pressed()
{
    switch (hw.activeButton)
    {
        case BUTTON_STOP:  return io.buttonStop;
        case BUTTON_LEFT:  return io.buttonLeft;
        case BUTTON_RIGHT: return io.buttonRight;
        case BUTTON_GO:    return io.buttonGo;
        default:           return false;
    }
}

void read_end_switches() {}

static void reference_run()
{
    if (!refRunActive)
        return;

    switch (refRunState)
    {
        case RefRunState::REF_START:
            refRunVMax = profileVMin + 20;
            if (refRunVMax > V_MAX_LIMIT)
                refRunVMax = V_MAX_LIMIT;

            refCoarseCaptured = false;
            refPreciseRightCaptured = false;
            refPreciseCaptured = false;
            refSecondPreciseCaptured = false;
            refMeasureArmed = false;
            refMeasurePass = 0;
            refCoarsePosition = 0;
            refRightEndPosition = 0;
            refPreciseRight = 0;
            refPreciseLeft = 0;
            refPreciseLength = 0;
            refPreciseCenter = 0;
            refPreciseRight2 = 0;
            refPreciseLeft2 = 0;
            refPreciseLength2 = 0;
            refPreciseCenter2 = 0;
            refApproachTarget = 0;
            refShiftTarget = 0;

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
                measurement.position = MAX_ALLOWED_STEPS / 2;

                if (!refLongRun)
                {
                    io.ledLeft = false;
                    io.ledRight = false;
                    updateOut = true;
                    hw.displayDirty = true;
                    refRunActive = false;
                    refRunState = RefRunState::IDLE;
                    Serial.println(F("REF: SHORT VALID"));

                    selectedTrack = 3;
                    trackMode = true;

                    if (localSequenceActive)
                    {
                        localSequenceActive = false;
                        localTrackActive = true;
                    }

                    serialDisplayDirty = true;
                    start_position_move(trackPosition[2]);
                    break;
                }

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
                profileRun = false;
                refRightEndPosition = measurement.position;

                // Die Groberkennung liegt an der linken Fahnenkante.
                // Wir fahren von LSR zügig zurück in die Nähe der Fahne,
                // aber mit Sicherheitsabstand rechts davon.
                refApproachTarget = refCoarsePosition + 1200;
                if (refApproachTarget >= refRightEndPosition)
                    refApproachTarget =
                        refRightEndPosition > 100
                            ? refRightEndPosition - 100
                            : 0;

                io.dir = false;
                updateOut = true;
                set_step_frequency(refRunVMax);
                stepRun = true;

                refRunState = RefRunState::REF_APPROACH_REF;
            }
            break;

        case RefRunState::REF_APPROACH_REF:
            if (measurement.position <= refApproachTarget)
            {
                stepRun = false;
                profileRun = false;

                noInterrupts();
                refLastState =
                    ((*LSREF.inputRegister) & LSREF.mask) != 0;
                refPreciseRightCaptured = false;
                refPreciseCaptured = false;
                refMeasurePass = 1;
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

                refShiftTarget =
                    refPreciseLeft > 500
                        ? refPreciseLeft - 500
                        : 0;

                io.dir = false;
                updateOut = true;
                set_step_frequency(V_MIN_LIMIT);
                stepRun = true;

                refRunState = RefRunState::REF_SHIFT_LEFT;
            }
            break;

        case RefRunState::REF_SHIFT_LEFT:
            if (measurement.position <= refShiftTarget)
            {
                stepRun = false;
                profileRun = false;

                noInterrupts();
                refLastState =
                    ((*LSREF.inputRegister) & LSREF.mask) != 0;
                refMeasurePass = 2;
                refMeasureArmed = true;
                refSecondPreciseCaptured = false;
                measurement.refValid = false;
                interrupts();

                io.dir = true;
                updateOut = true;
                set_step_frequency(V_MIN_LIMIT);
                stepRun = true;

                refRunState = RefRunState::REF_MEASURE_LEFT;
            }
            break;

        case RefRunState::REF_MEASURE_LEFT:
            if (refSecondPreciseCaptured)
            {
                stepRun = false;
                profileRun = false;
                refRunState = RefRunState::REF_CALCULATE;
            }
            break;

        case RefRunState::REF_CALCULATE:
            {
                const int16_t playLeft =
                    static_cast<int16_t>(refPreciseLeft2) -
                    static_cast<int16_t>(refPreciseLeft);

                const int16_t playRight =
                    static_cast<int16_t>(refPreciseRight2) -
                    static_cast<int16_t>(refPreciseRight);

                const int16_t centerShift =
                    static_cast<int16_t>(refPreciseCenter2) -
                    static_cast<int16_t>(refPreciseCenter);

                Serial.print(F("REF: DATA COARSE="));
                Serial.print(refCoarsePosition);
                Serial.print(F(" RL_R="));
                Serial.print(refPreciseRight);
                Serial.print(F(" RL_L="));
                Serial.print(refPreciseLeft);
                Serial.print(F(" RL_LEN="));
                Serial.print(refPreciseLength);
                Serial.print(F(" LR_L="));
                Serial.print(refPreciseLeft2);
                Serial.print(F(" LR_R="));
                Serial.print(refPreciseRight2);
                Serial.print(F(" LR_LEN="));
                Serial.print(refPreciseLength2);
                Serial.print(F(" PLAY_L="));
                Serial.print(playLeft);
                Serial.print(F(" PLAY_R="));
                Serial.print(playRight);
                Serial.print(F(" CENTER_SHIFT="));
                Serial.print(centerShift);
                Serial.print(F(" LSR="));
                Serial.print(refRightEndPosition);
                Serial.print(F(" SPAN="));
                Serial.println(
                    refRightEndPosition > 1000
                        ? refRightEndPosition - 1000
                        : 0);

                refRunActive = false;
                hw.displayDirty = true;
                Serial.println(F("REF: VALID"));

                if (localSequenceActive && refLongRun)
                {
                    localSequenceActive = false;
                    localTrackActive = true;
                    trackMode = true;
                    selectedTrack = 3;
                }

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

        case RefRunState::REF_RIGHT_END:
        case RefRunState::REF_SLOW_REF:
        case RefRunState::REF_MEASURE_RIGHT:
        case RefRunState::IDLE:
        default:
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
        measurement.position = 1000;
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
    measurement.position = MAX_ALLOWED_STEPS / 2;
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
    refSecondPreciseCaptured = false;
    refMeasurePass = 0;
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

    // Im lokalen Bereitschaftszustand wird der gewählte Startmodus
    // bewusst groß dargestellt. Nach dem Start bleibt die normale
    // Diagnoseanzeige erhalten.
    if (!localTrackActive && !localSequenceActive && !io.ena)
    {
        display.setTextSize(2);
        display.setCursor(0, 0);

        if (localStartMode == LocalStartMode::TRACK)
            display.println(F("TRACK"));
        else
            display.println(F("LONG REF"));

        display.setTextSize(1);
        display.setCursor(0, 24);
        display.println(F("BLUE=MODE  GREEN=START"));
        display.display();
        display.setTextSize(1);
        return;
    }

    display.setTextSize(1);
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
    display.print(F(" TRK: "));
    if (trackMode)
        display.print(selectedTrack);
    else
        display.print('-');
    display.println();

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
                        refLongRun = false;
                        refRunState = RefRunState::IDLE;
                        localSequenceActive = false;
                        localTrackActive = false;
                        set_active_button(BUTTON_STOP);
                        break;

                    case BUTTON_GO:
                        hw.ena = true;
                        io.ena = true;
                        stepRun = false;
                        set_step_frequency(hw.frequency);
                        set_active_button(BUTTON_GO);

                        if (localTrackActive)
                        {
                            if (selectedTrack >= 1 && selectedTrack <= TRACK_COUNT)
                            {
                                Serial.print(F("LOCAL: GO BG"));
                                Serial.print(selectedTrack);
                                Serial.print(F(" POS="));
                                Serial.println(trackPosition[selectedTrack - 1]);

                                start_position_move(trackPosition[selectedTrack - 1]);
                            }
                            break;
                        }

                        if (!localSequenceActive)
                        {
                            localSequenceActive = true;
                            refRunState = RefRunState::REF_START;
                            refLongRun =
                                (localStartMode == LocalStartMode::LONG_REF);
                            serialDisplayDirty = true;

                            if (refLongRun)
                                Serial.println(F("LOCAL: LONG REF"));
                            else
                                Serial.println(F("LOCAL: TRACK"));

                            refRunActive = true;
                        }
                        break;

                    case BUTTON_LEFT:
                        if (localSequenceActive || refRunActive)
                            break;

                        if (localTrackActive)
                        {
                            if (selectedTrack >= TRACK_COUNT)
                                selectedTrack = 1;
                            else
                                ++selectedTrack;

                            trackMode = true;
                            serialDisplayDirty = true;

                            Serial.print(F("LOCAL: SELECT BG"));
                            Serial.print(selectedTrack);
                            Serial.print(F(" POS="));
                            Serial.println(trackPosition[selectedTrack - 1]);
                            hw.displayDirty = true;
                            break;
                        }

                        if (io.ena)
                            break;

                        localStartMode =
                            (localStartMode == LocalStartMode::TRACK)
                                ? LocalStartMode::LONG_REF
                                : LocalStartMode::TRACK;
                        serialDisplayDirty = true;
                        hw.displayDirty = true;
                        set_active_button(BUTTON_LEFT);
                        break;

                    case BUTTON_RIGHT:
                        if (localSequenceActive || refRunActive)
                            break;

                        if (localTrackActive)
                        {
                            if (selectedTrack <= 1)
                                selectedTrack = TRACK_COUNT;
                            else
                                --selectedTrack;

                            trackMode = true;
                            serialDisplayDirty = true;

                            Serial.print(F("LOCAL: SELECT BG"));
                            Serial.print(selectedTrack);
                            Serial.print(F(" POS="));
                            Serial.println(trackPosition[selectedTrack - 1]);
                            hw.displayDirty = true;
                            break;
                        }

                        if (io.ena)
                            break;

                        localStartMode =
                            (localStartMode == LocalStartMode::TRACK)
                                ? LocalStartMode::LONG_REF
                                : LocalStartMode::TRACK;
                        serialDisplayDirty = true;
                        hw.displayDirty = true;
                        set_active_button(BUTTON_RIGHT);
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