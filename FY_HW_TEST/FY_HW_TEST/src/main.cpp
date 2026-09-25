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
static uint32_t tSerial   = 0;

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


// -----------------------------------------------------------------------------
// PCF8574
// -----------------------------------------------------------------------------

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

    // LEDs off
    expander.digitalWrite(LED_STOP_PIN, HIGH);
    expander.digitalWrite(LED_GO_PIN, HIGH);
    expander.digitalWrite(LED_LEFT_PIN, HIGH);
    expander.digitalWrite(LED_RIGHT_PIN, HIGH);

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

    // One event per scan. Priority: STOP > LEFT > RIGHT > GO.
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

    // Only the active command LED is on.
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
    digitalWrite(MOTOR_ENA,  hw.ena  ? HIGH : LOW);
    digitalWrite(MOTOR_DIR,  hw.dir  ? HIGH : LOW);
    digitalWrite(MOTOR_PULS, hw.puls ? HIGH : LOW);
}


// -----------------------------------------------------------------------------
// Display
// -----------------------------------------------------------------------------

void update_display()
{
    display.clearDisplay();
    display.setCursor(0, 0);

    display.println(F("FY HW TEST"));

    display.print(F("L:"));
    display.print(hw.limitLeft ? '1' : '0');
    display.print(F(" R:"));
    display.print(hw.limitRight ? '1' : '0');
    display.print(F(" Ref:"));
    display.print(hw.reference ? '1' : '0');
    display.print(F(" H:"));
    display.println(hw.hall ? '1' : '0');

    display.print(F("F:"));
    display.print(hw.frequency);
    display.print(F("Hz S:"));
    display.println(hw.steps);

    display.display();
}


// -----------------------------------------------------------------------------
// Serial
// -----------------------------------------------------------------------------

void update_serial()
{
    Serial.print(F("BTN="));
    Serial.print(hw.button);
    Serial.print(F(" ACTIVE="));
    Serial.print(hw.activeButton);
    Serial.print(F(" BUSY="));
    Serial.print(hw.busy);
    Serial.print(F(" ENA="));
    Serial.print(hw.ena);
    Serial.print(F(" DIR="));
    Serial.print(hw.dir);
    Serial.print(F(" L="));
    Serial.print(hw.limitLeft);
    Serial.print(F(" R="));
    Serial.print(hw.limitRight);
    Serial.print(F(" REF="));
    Serial.print(hw.reference);
    Serial.print(F(" HALL="));
    Serial.println(hw.hall);
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

    // Safe initial state
    hw.ena  = false;
    hw.dir  = false;
    hw.puls = false;
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
            // STOP is always accepted. While busy, all other buttons are
            // ignored until the active action has completed.
            if (event == BUTTON_STOP || !hw.busy)
            {
                hw.button = event;
                hw.displayDirty = true;

                switch (event)
                {
                    case BUTTON_STOP:
                        hw.ena = false;
                        hw.dir = false;
                        hw.puls = false;

                        // STOP is a completed action once ENA is safely off.
                        clear_active_button();
                        break;

                    case BUTTON_GO:
                        hw.ena = true;
                        hw.stepReload = 100;
                        hw.frequency = 100;
                        set_active_button(BUTTON_GO);
                        break;

                    case BUTTON_LEFT:
                        hw.dir = false;
                        hw.stepReload = 100;
                        hw.frequency = 100;
                        hw.steps = 100;
                        set_active_button(BUTTON_LEFT);
                        break;

                    case BUTTON_RIGHT:
                        hw.dir = true;
                        hw.stepReload = 100;
                        hw.frequency = 100;
                        hw.steps = 100;
                        set_active_button(BUTTON_RIGHT);
                        break;

                    default:
                        break;
                }

                // The physical press is consumed. The persistent action state
                // is kept separately in activeButton/busy.
                hw.button = BUTTON_NONE;
            }
        }
    }

    if (now - tSwitches >= SWITCH_INTERVAL_MS)
    {
        tSwitches = now;
        read_end_switches();
    }

    if (now - tSerial >= SERIAL_INTERVAL_MS)
    {
        tSerial = now;
        update_serial();
    }

    if (now - tDisplay >= DISPLAY_INTERVAL_MS || hw.displayDirty)
    {
        tDisplay = now;
        update_display();
        hw.displayDirty = false;
    }

    update_outputs();
}
