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
// Button / LED mapping test
// -----------------------------------------------------------------------------

static const uint8_t MAPPING_LED_PINS[4] =
{
    LED_GO_PIN,
    LED_STOP_PIN,
    LED_LEFT_PIN,
    LED_RIGHT_PIN
};

static const char* const MAPPING_NAMES[4] =
{
    "GREEN / GO",
    "RED / STOP",
    "BLUE 1 / LEFT",
    "BLUE 2 / RIGHT"
};

static void all_mapping_leds_off()
{
    for (uint8_t i = 0; i < 4; ++i)
        expander.digitalWrite(MAPPING_LED_PINS[i], HIGH);
}

static void wait_for_all_buttons_released()
{
    while (expander.digitalRead(P4) == LOW ||
           expander.digitalRead(P5) == LOW ||
           expander.digitalRead(P6) == LOW ||
           expander.digitalRead(P7) == LOW)
    {
        delay(10);
    }
}

static int8_t read_any_mapping_button()
{
    const uint8_t pins[4] = { P4, P5, P6, P7 };

    for (uint8_t i = 0; i < 4; ++i)
    {
        if (expander.digitalRead(pins[i]) == LOW)
            return static_cast<int8_t>(pins[i]);
    }

    return -1;
}

static void print_mapping_pin(uint8_t pin)
{
    Serial.print('P');
    Serial.print(pin);
}

static void run_button_led_mapping()
{
    Serial.println();
    Serial.println(F("=== BUTTON / LED MAPPING TEST ==="));
    Serial.println(F("The test uses the CURRENT assumed LED mapping."));
    Serial.println(F("For each function, press the PHYSICAL button"));
    Serial.println(F("belonging to that colour/function."));
    Serial.println();

    wait_for_all_buttons_released();

    int8_t detected[4] = { -1, -1, -1, -1 };

    for (uint8_t i = 0; i < 4; ++i)
    {
        all_mapping_leds_off();
        expander.digitalWrite(MAPPING_LED_PINS[i], LOW);

        Serial.print(F("Please press the GREEN/RED/BLUE button for: "));
        Serial.println(MAPPING_NAMES[i]);

        int8_t buttonPin = -1;

        while (buttonPin < 0)
        {
            buttonPin = read_any_mapping_button();
            delay(10);
        }

        detected[i] = buttonPin;

        Serial.print(F("Detected physical button: "));
        print_mapping_pin(static_cast<uint8_t>(buttonPin));
        Serial.println();

        expander.digitalWrite(MAPPING_LED_PINS[i], HIGH);
        wait_for_all_buttons_released();
        delay(250);
    }

    Serial.println();
    Serial.println(F("=== MAPPING RESULT ==="));
    Serial.println(F("Function | assumed LED | detected BUTTON"));

    for (uint8_t i = 0; i < 4; ++i)
    {
        Serial.print(MAPPING_NAMES[i]);
        Serial.print(F(" | LED P"));
        Serial.print(MAPPING_LED_PINS[i]);
        Serial.print(F(" | BUTTON P"));
        Serial.println(detected[i]);
    }

    Serial.println();
    Serial.println(F("Recommended fixed assignment:"));

    Serial.print(F("  GREEN / GO   -> BUTTON P"));
    Serial.println(detected[0]);

    Serial.print(F("  RED / STOP   -> BUTTON P"));
    Serial.println(detected[1]);

    Serial.print(F("  BLUE 1 / LEFT -> BUTTON P"));
    Serial.println(detected[2]);

    Serial.print(F("  BLUE 2 / RIGHT -> BUTTON P"));
    Serial.println(detected[3]);

    Serial.println(F("=== MAPPING TEST DONE ==="));
    Serial.println();

    all_mapping_leds_off();
    wait_for_all_buttons_released();
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

static void handle_serial_commands()
{
    while (Serial.available() > 0)
    {
        const char command = Serial.read();

        if (command == 'm' || command == 'M')
            run_button_led_mapping();
    }
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
    Serial.println(F("Send 'M' to start button / LED mapping test."));
}


// -----------------------------------------------------------------------------
// Main loop - simple cooperative scheduler
// -----------------------------------------------------------------------------

void loop()
{
    const uint32_t now = millis();

    handle_serial_commands();

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
