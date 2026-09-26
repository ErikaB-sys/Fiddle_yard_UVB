#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <PCF8574.h>

#include "HW_Test.h"

// -----------------------------------------------------------------------------
// Hardware
// -----------------------------------------------------------------------------

Adafruit_SSD1306 display(128, 32, &Wire, -1);
PCF8574 expander(PORT_EXPANDER_ADDRESS);

HWIOState io;
HWTestState hw;

// Scheduler timestamps
static uint32_t tButtons  = 0;
static uint32_t tSwitches = 0;
static uint32_t tDisplay  = 0;

// Previous button state for edge detection
static bool lastStop  = false;
static bool lastLeft  = false;
static bool lastRight = false;
static bool lastGo    = false;


// -----------------------------------------------------------------------------
// I2C
// -----------------------------------------------------------------------------

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


// -----------------------------------------------------------------------------
// OLED
// -----------------------------------------------------------------------------

void initOLED()
{
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println(F("Error initializing OLED!"));

        while (true)
        {
        }
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


// Forward declaration used during expander initialization.
static void all_mapping_leds_off();

// -----------------------------------------------------------------------------
// PCF8574
// -----------------------------------------------------------------------------

void init_expander()
{
    // Configure the PCF8574 before begin(). INPUT is sufficient for the
    // quasi-bidirectional button inputs; no manual HIGH writes are needed.
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

        while (true)
        {
        }
    }
}


// -----------------------------------------------------------------------------
// Buttons
// -----------------------------------------------------------------------------

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


// -----------------------------------------------------------------------------
// Action / LED state
// -----------------------------------------------------------------------------

void set_active_button(ButtonEvent event)
{
    hw.activeButton = event;
    hw.busy = (event != BUTTON_NONE);
    hw.button = event;

    // Green/red indicate the latched motor enable state.
    // Blue LEDs indicate the currently active movement button.
    io.ledStop  = !io.ena;
    io.ledGo    = io.ena;
    io.ledLeft  = (event == BUTTON_LEFT);
    io.ledRight = (event == BUTTON_RIGHT);

    hw.displayDirty = true;
}

void clear_active_button()
{
    hw.activeButton = BUTTON_NONE;
    hw.busy = false;
    hw.button = BUTTON_NONE;

    // Keep green/red as motor-state indicators.
    // Movement LEDs are only active while the button is held.
    io.ledStop = !io.ena;
    io.ledGo = io.ena;
    io.ledLeft = false;
    io.ledRight = false;

    hw.displayDirty = true;
}


// Return the current physical state of the active button.
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

// -----------------------------------------------------------------------------
// End switches
// -----------------------------------------------------------------------------

void read_end_switches() { }


// -----------------------------------------------------------------------------
// Outputs
// -----------------------------------------------------------------------------

void update_outputs()
{
    digitalWrite(MOTOR_ENA, io.ena ? HIGH : LOW);
    digitalWrite(MOTOR_DIR, io.dir ? HIGH : LOW);

    expander.digitalWrite(LED_STOP_PIN, io.ledStop ? LOW : HIGH);
    expander.digitalWrite(LED_GO_PIN, io.ledGo ? LOW : HIGH);
    expander.digitalWrite(LED_LEFT_PIN, io.ledLeft ? LOW : HIGH);
    expander.digitalWrite(LED_RIGHT_PIN, io.ledRight ? LOW : HIGH);
}

// -----------------------------------------------------------------------------
// Timer1 STEP generator
// -----------------------------------------------------------------------------
// 20 Hz interrupt -> 10 Hz STEP square wave. Intentionally slow for the
// hardware smoke test so the signal is easy to observe.

static volatile bool stepRun = false;
static volatile bool stepLevel = false;

// -----------------------------------------------------------------------------
// Mechanik-Messung
// -----------------------------------------------------------------------------
// Die Messwerte werden ausschließlich in der Timer1-ISR erzeugt.
// Das Hauptprogramm holt sie erst im Stillstand per get_measurement() ab.
volatile HWMeasurement measurement;

static volatile bool refLastState = false;
static volatile bool refStartValid = false;
static volatile bool leftReferenceApplied = false;
static volatile bool rightLengthCaptured = false;

ISR(TIMER1_COMPA_vect)
{
    // Sensoren direkt im ISR-Kontext lesen.
    // Pin, Port und Bitmaske sind gemeinsam in HW_Test.h definiert.
    // Endschalter: aktiv HIGH (LOW = Lichtschranke frei).
    // Referenzfahne: HIGH innerhalb der Fahne.
    const bool limitRight = ((*LSR.inputRegister) & LSR.mask) != 0;
    const bool reference  = ((*LSREF.inputRegister) & LSREF.mask) != 0;
    const bool limitLeft  = ((*LSL.inputRegister) & LSL.mask) != 0;

    // Endpositionen einmalig erfassen.
    // Links wird der Positionszähler auf 1000 gesetzt.
    // Rechts wird daraus der Gesamtweg Links -> Rechts berechnet.
    if (limitLeft && !leftReferenceApplied)
    {
        measurement.position = 1000;
        measurement.refStart = 0;
        measurement.refEnd = 0;
        measurement.refLength = 0;
        measurement.totalLength = 0;
        measurement.refValid = false;
        refStartValid = false;
        leftReferenceApplied = true;
    }
    else if (!limitLeft)
    {
        leftReferenceApplied = false;
    }

    if (limitRight && !rightLengthCaptured)
    {
        if (measurement.position >= 1000)
            measurement.totalLength = measurement.position - 1000;

        rightLengthCaptured = true;
    }
    else if (!limitRight)
    {
        rightLengthCaptured = false;
    }

    // Sofortiger Hardware-Stopp in die ausgelöste Richtung.
    if (stepRun)
    {
        if ((!io.dir && limitLeft) || (io.dir && limitRight))
        {
            stepRun = false;
            stepLevel = false;
            PORTD &= ~_BV(PD3);
            PORTB &= ~_BV(PB5);
            return;
        }
    }

    if (!stepRun)
    {
        stepLevel = false;
        PORTD &= ~_BV(PD3);
        PORTB &= ~_BV(PB5);
        return;
    }

    stepLevel = !stepLevel;

    if (stepLevel)
    {
        // STEP HIGH = Beginn des Schrittes.
        PORTD |= _BV(PD3);
        PORTB |= _BV(PB5);
    }
    else
    {
        // STEP LOW = Schritt abgeschlossen.
        PORTD &= ~_BV(PD3);
        PORTB &= ~_BV(PB5);

        // Position logisch zählen: links -> rechts aufwärts.
        if (io.dir)
            measurement.position++;
        else if (measurement.position > 0)
            measurement.position--;

        // Referenzfahne über die Flanken vermessen.
        // LOW -> HIGH: Startpunkt merken.
        // HIGH -> LOW: nur bei steigender Position als gültige Länge übernehmen.
        if (!refLastState && reference)
        {
            measurement.refStart = measurement.position;
            refStartValid = true;
            measurement.refValid = false;
        }
        else if (refLastState && !reference)
        {
            if (refStartValid && measurement.position > measurement.refStart)
            {
                measurement.refEnd = measurement.position;
                measurement.refLength =
                    measurement.refEnd - measurement.refStart;
                measurement.refValid = true;
            }
        }

        refLastState = reference;

    }
}

void set_step_frequency(uint16_t frequency)
{
    if (frequency == 0)
    {
        return;
    }

    // 16 MHz / 1024 timer clock. The ISR toggles STEP, so the interrupt
    // frequency is twice the requested STEP frequency.
    uint32_t denominator = 2UL * frequency;
    uint32_t reload = ((F_CPU / 1024UL) + (denominator / 2UL)) / denominator - 1UL;

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
    TCCR1B |= _BV(CS12) | _BV(CS10);
    TIMSK1 |= _BV(OCIE1A);
    stepRun = false;
    stepLevel = false;
    measurement.position = 1000;
    measurement.refStart = 0;
    measurement.refEnd = 0;
    measurement.refLength = 0;
    measurement.totalLength = 0;
    measurement.refValid = false;
    refStartValid = false;
    leftReferenceApplied = false;
    rightLengthCaptured = false;
    refLastState = (PINB & _BV(PB3)) != 0;
    PORTD &= ~_BV(PD3);
    PORTB &= ~_BV(PB5);
    interrupts();

    set_step_frequency(STEP_FREQUENCIES[0]);
}


// -----------------------------------------------------------------------------
// Messwerte aus der ISR abholen
// -----------------------------------------------------------------------------

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

// -----------------------------------------------------------------------------
// Display
// -----------------------------------------------------------------------------

void update_display()
{
    display.clearDisplay();
    display.setCursor(0, 0);

    // Messwerte erst bei stehender Bühne übernehmen.
    static HWMeasurement displayMeasurement;

    if (!stepRun)
    {
        get_measurement(displayMeasurement);
    }

    const HWMeasurement& m = displayMeasurement;

    // Zeile 1: Motorzustand + Schrittfrequenz.
    display.print(F("M:"));
    display.print(io.ena ? F("ON ") : F("OFF"));
    display.print(hw.frequency);
    display.println(F("Hz"));

    // Zeile 2: gemessene Referenzfahne + Gesamtweg L -> R.
    display.print(F("REF:"));
    if (m.refValid)
        display.print(m.refLength);
    else
        display.print(F("---"));

    display.print(F(" L-R:"));
    if (m.totalLength > 0)
        display.println(m.totalLength);
    else
        display.println(F("---"));

    // Zeile 3: LS-Zustand aus dem Prozessbild.
    // L/R = Endschalter, F = Referenzfahne, H = Hall-Sensor.
    display.print(F("LS:L"));
    display.print(io.limitLeft ? '1' : '0');
    display.print(F(" R"));
    display.print(io.limitRight ? '1' : '0');
    display.print(F(" F"));
    display.print(io.reference ? '1' : '0');
    display.print(F(" H"));
    display.println(io.hall ? '1' : '0');

    display.display();
}
// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------

void setup()
{
    Serial.begin(9600);

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
    hw.stepReload = 100;
    hw.frequency = STEP_FREQUENCIES[0];
    hw.steps = 100;

    clear_active_button();

    Serial.println(F("init done"));
}


// -----------------------------------------------------------------------------
// Main loop - simple cooperative scheduler
// -----------------------------------------------------------------------------

void loop()
{
    const uint32_t now = millis();

    if (now - tButtons >= BUTTON_INTERVAL_MS)
    {
        tButtons = now;

        read_IO();
        const ButtonEvent event = read_buttons();

        if (event != BUTTON_NONE)
        {
            if (event == BUTTON_STOP || !hw.busy)
            {
                hw.displayDirty = true;

                switch (event)
                {
                    case BUTTON_STOP:
                        hw.ena = false;
                        io.ena = false;
                        stepRun = false;
                        set_active_button(BUTTON_STOP);
                        break;

                    case BUTTON_GO:
                        hw.ena = true;
                        io.ena = true;
                        stepRun = false;
                        // Keep the frequency selected while ENA was OFF.
                        set_step_frequency(hw.frequency);
                        set_active_button(BUTTON_GO);
                        break;

                    case BUTTON_LEFT:
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
                            stepRun = true;
                            set_step_frequency(hw.frequency);
                            set_active_button(BUTTON_LEFT);
                        }
                        break;

                    case BUTTON_RIGHT:
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
                            stepRun = true;
                            set_step_frequency(hw.frequency);
                            set_active_button(BUTTON_RIGHT);
                        }
                        break;

                    default:
                        break;
                }

            }
        }
    }

    if (now - tSwitches >= SWITCH_INTERVAL_MS)
    {
        tSwitches = now;
        read_end_switches();
    }

    // The hardware test is deliberately press-and-hold:
    // press -> LED/action ON, release -> LED/action OFF.
    if (hw.busy && !active_button_pressed())
    {
        if (hw.activeButton == BUTTON_LEFT ||
            hw.activeButton == BUTTON_RIGHT)
        {
            stepRun = false;
        }

        clear_active_button();
    }

    if (now - tDisplay >= DISPLAY_INTERVAL_MS || hw.displayDirty)
    {
        tDisplay = now;
        update_display();
        hw.displayDirty = false;
    }

    update_outputs();
}
