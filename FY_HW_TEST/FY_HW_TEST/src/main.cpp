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

ButtonEvent read_buttons()
{
    const bool stop  = (expander.digitalRead(BUTTON_STOP_PIN)  == LOW);
    const bool left  = (expander.digitalRead(BUTTON_LEFT_PIN)  == LOW);
    const bool right = (expander.digitalRead(BUTTON_RIGHT_PIN) == LOW);
    const bool go    = (expander.digitalRead(BUTTON_GO_PIN)    == LOW);

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

    expander.digitalWrite(LED_STOP_PIN,  event == BUTTON_STOP  ? LOW : HIGH);
    expander.digitalWrite(LED_GO_PIN,    event == BUTTON_GO    ? LOW : HIGH);
    expander.digitalWrite(LED_LEFT_PIN,  event == BUTTON_LEFT  ? LOW : HIGH);
    expander.digitalWrite(LED_RIGHT_PIN, event == BUTTON_RIGHT ? LOW : HIGH);

    hw.displayDirty = true;
}

void clear_active_button()
{
    hw.activeButton = BUTTON_NONE;
    hw.busy = false;

    expander.digitalWrite(LED_STOP_PIN, HIGH);
    expander.digitalWrite(LED_GO_PIN, HIGH);
    expander.digitalWrite(LED_LEFT_PIN, HIGH);
    expander.digitalWrite(LED_RIGHT_PIN, HIGH);

    hw.displayDirty = true;
}


// -----------------------------------------------------------------------------
// End switches
// -----------------------------------------------------------------------------

void read_end_switches()
{
    const bool left  = digitalRead(LSL);
    const bool right = digitalRead(LSR);
    const bool ref   = digitalRead(LSREF);
    const bool hall  = digitalRead(LS4);

    if (left != hw.limitLeft ||
        right != hw.limitRight ||
        ref != hw.reference ||
        hall != hw.hall)
    {
        hw.displayDirty = true;
    }

    hw.limitLeft  = left;
    hw.limitRight = right;
    hw.reference  = ref;
    hw.hall       = hall;
}


// -----------------------------------------------------------------------------
// Outputs
// -----------------------------------------------------------------------------

void update_outputs()
{
    digitalWrite(MOTOR_ENA, hw.ena ? HIGH : LOW);
    digitalWrite(MOTOR_DIR, hw.dir ? HIGH : LOW);
}

// -----------------------------------------------------------------------------
// Timer1 STEP generator
// -----------------------------------------------------------------------------
// 20 Hz interrupt -> 10 Hz STEP square wave. Intentionally slow for the
// hardware smoke test so the signal is easy to observe.

static volatile bool stepRun = false;
static volatile bool stepLevel = false;

ISR(TIMER1_COMPA_vect)
{
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
        PORTD |= _BV(PD3);
        PORTB |= _BV(PB5);
    }
    else
    {
        PORTD &= ~_BV(PD3);
        PORTB &= ~_BV(PB5);
    }
}

void init_step_timer()
{
    noInterrupts();
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;
    OCR1A = 780;
    TCCR1B |= _BV(WGM12);
    TCCR1B |= _BV(CS12) | _BV(CS10);
    TIMSK1 |= _BV(OCIE1A);
    stepRun = false;
    stepLevel = false;
    PORTD &= ~_BV(PD3);
    PORTB &= ~_BV(PB5);
    interrupts();
}


// -----------------------------------------------------------------------------
// Display
// -----------------------------------------------------------------------------

void update_display()
{
    display.clearDisplay();
    display.setCursor(0, 0);

    display.print(F("G:"));
    display.print(expander.digitalRead(BUTTON_GO_PIN) == LOW ? '1' : '0');
    display.print(F(" S:"));
    display.print(expander.digitalRead(BUTTON_STOP_PIN) == LOW ? '1' : '0');
    display.print(F(" B1:"));
    display.print(expander.digitalRead(BUTTON_LEFT_PIN) == LOW ? '1' : '0');
    display.print(F(" B2:"));
    display.println(expander.digitalRead(BUTTON_RIGHT_PIN) == LOW ? '1' : '0');

    display.print(F("ENA:"));
    display.print(hw.ena ? '1' : '0');
    display.print(F(" DIR:"));
    display.print(hw.dir ? '1' : '0');
    display.print(F(" STEP:"));
    display.println(stepRun ? '1' : '0');

    display.print(F("STEP "));
    display.print(hw.frequency);
    display.println(F(" Hz"));

    display.print(F("L:"));
    display.print(hw.limitLeft ? '1' : '0');
    display.print(F(" R:"));
    display.print(hw.limitRight ? '1' : '0');
    display.print(F(" Ref:"));
    display.print(hw.reference ? '1' : '0');
    display.print(F(" H:"));
    display.println(hw.hall ? '1' : '0');

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

    pinMode(LSL, INPUT);
    pinMode(LSR, INPUT);
    pinMode(LSREF, INPUT);
    pinMode(LS4, INPUT);

    pinMode(MOTOR_ENA, OUTPUT);
    pinMode(MOTOR_PULS, OUTPUT);
    pinMode(MOTOR_DIR, OUTPUT);
    pinMode(LED_BUILTIN_PIN, OUTPUT);

    init_step_timer();

    hw.ena  = false;
    hw.dir  = false;
    hw.puls = false;
    stepRun = false;
    hw.stepReload = 100;
    hw.frequency = 100;
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

        const ButtonEvent event = read_buttons();

        if (event != BUTTON_NONE)
        {
            if (event == BUTTON_STOP || !hw.busy)
            {
                hw.button = event;
                hw.displayDirty = true;

                switch (event)
                {
                    case BUTTON_STOP:
                        hw.ena = false;
                        stepRun = false;
                        clear_active_button();
                        break;

                    case BUTTON_GO:
                        hw.ena = true;
                        stepRun = false;
                        set_active_button(BUTTON_GO);
                        break;

                    case BUTTON_LEFT:
                        hw.ena = true;
                        hw.dir = false;
                        stepRun = true;
                        hw.frequency = 10;
                        set_active_button(BUTTON_LEFT);
                        break;

                    case BUTTON_RIGHT:
                        hw.ena = true;
                        hw.dir = true;
                        stepRun = true;
                        hw.frequency = 10;
                        set_active_button(BUTTON_RIGHT);
                        break;

                    default:
                        break;
                }

                hw.button = BUTTON_NONE;
            }
        }
    }

    if (now - tSwitches >= SWITCH_INTERVAL_MS)
    {
        tSwitches = now;
        read_end_switches();
    }

    if (now - tDisplay >= DISPLAY_INTERVAL_MS || hw.displayDirty)
    {
        tDisplay = now;
        update_display();
        hw.displayDirty = false;
    }

    update_outputs();
}
