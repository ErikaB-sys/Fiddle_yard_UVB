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
    expander.pinMode(BUTTON_STOP_PIN, INPUT);
    expander.pinMode(BUTTON_GO_PIN, INPUT);
    expander.pinMode(BUTTON_LEFT_PIN, INPUT);
    expander.pinMode(BUTTON_RIGHT_PIN, INPUT);

    // PCF8574 inputs are quasi-bidirectional. Writing HIGH releases the
    // corresponding pin and enables its weak internal pull-up.
    expander.digitalWrite(BUTTON_STOP_PIN, HIGH);
    expander.digitalWrite(BUTTON_GO_PIN, HIGH);
    expander.digitalWrite(BUTTON_LEFT_PIN, HIGH);
    expander.digitalWrite(BUTTON_RIGHT_PIN, HIGH);

    expander.pinMode(LED_STOP_PIN, OUTPUT);
    expander.pinMode(LED_GO_PIN, OUTPUT);
    expander.pinMode(LED_LEFT_PIN, OUTPUT);
    expander.pinMode(LED_RIGHT_PIN, OUTPUT);

    all_mapping_leds_off();

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

static const uint8_t MAPPING_BUTTON_PINS[4] = { P4, P5, P6, P7 };

constexpr uint32_t MAPPING_DEBOUNCE_MS = 50;
constexpr uint32_t MAPPING_LED_INTERVAL_MS = 2000;

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

static bool wait_for_stable_button_state(uint8_t pin, bool pressed)
{
    const uint32_t start = millis();

    while (millis() - start < MAPPING_DEBOUNCE_MS)
    {
        const bool state = (expander.digitalRead(pin) == LOW);

        if (state != pressed)
            return false;

        delay(10);
    }

    return true;
}

static int8_t read_any_mapping_button()
{
    for (uint8_t i = 0; i < 4; ++i)
    {
        if (expander.digitalRead(MAPPING_BUTTON_PINS[i]) == LOW)
            return static_cast<int8_t>(MAPPING_BUTTON_PINS[i]);
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
    // Blocking by design: while this test is running, the normal scheduler
    // and normal button handling do not run.
    int8_t detectedButtons[4] = { -1, -1, -1, -1 };
    int8_t detectedLeds[4]    = { -1, -1, -1, -1 };

    Serial.println();
    Serial.println(F("=== BUTTON / LED MAPPING TEST ==="));
    Serial.println();

    // Do not let a button already being held enter the test as a new press.
    wait_for_all_buttons_released();

    // -------------------------------------------------------------------------
    // Phase 1
    // -------------------------------------------------------------------------
    for (uint8_t function = 0; function < 4; ++function)
    {
        all_mapping_leds_off();

        Serial.print(F("Druecke bitte die Taste: "));
        Serial.println(MAPPING_NAMES[function]);
        Serial.println(F("Taste gedrueckt halten."));
        Serial.println(F("LEDs wechseln alle 2 Sekunden."));
        Serial.println(F("Loslassen, sobald die richtige LED leuchtet."));

        // Wait until any physical button is actually pressed.
        int8_t buttonPin = -1;

        while (buttonPin < 0)
        {
            const int8_t candidate = read_any_mapping_button();

            if (candidate >= 0 &&
                wait_for_stable_button_state(static_cast<uint8_t>(candidate), true))
            {
                buttonPin = candidate;
            }

            delay(10);
        }

        detectedButtons[function] = buttonPin;

        Serial.print(F("PRESS erkannt: "));
        print_mapping_pin(static_cast<uint8_t>(buttonPin));
        Serial.println();

        // Start immediately with LED 0.
        uint8_t ledIndex = 0;
        expander.digitalWrite(MAPPING_LED_PINS[ledIndex], LOW);

        // Keep the button pressed. Change the LED every 2 seconds.
        // Release is checked every 10 ms and must be stable for 50 ms.
        uint32_t nextLedChange = millis() + MAPPING_LED_INTERVAL_MS;

        while (true)
        {
            const bool pressed =
                (expander.digitalRead(static_cast<uint8_t>(buttonPin)) == LOW);

            if (!pressed &&
                wait_for_stable_button_state(static_cast<uint8_t>(buttonPin), false))
            {
                detectedLeds[function] = static_cast<int8_t>(ledIndex);

                Serial.print(F("RELEASE erkannt: "));
                print_mapping_pin(static_cast<uint8_t>(buttonPin));
                Serial.println();

                Serial.print(F("Auswahl: "));
                Serial.println(MAPPING_NAMES[ledIndex]);
                break;
            }

            if (static_cast<int32_t>(millis() - nextLedChange) >= 0)
            {
                expander.digitalWrite(MAPPING_LED_PINS[ledIndex], HIGH);

                ledIndex++;
                if (ledIndex >= 4)
                    ledIndex = 0;

                expander.digitalWrite(MAPPING_LED_PINS[ledIndex], LOW);
                nextLedChange += MAPPING_LED_INTERVAL_MS;
            }

            delay(10);
        }

        all_mapping_leds_off();

        // Do not advance until the physical button is definitely released.
        wait_for_all_buttons_released();
        delay(200);
    }

    // -------------------------------------------------------------------------
    // Phase 2: counter-check
    // -------------------------------------------------------------------------
    Serial.println();
    Serial.println(F("=== GEGENPROBE ==="));
    Serial.println(F("Sechs zufaellige Tasten pruefen."));
    Serial.println();

    randomSeed(analogRead(A0));

    for (uint8_t test = 0; test < 6; ++test)
    {
        wait_for_all_buttons_released();
        all_mapping_leds_off();

        Serial.print(F("Test "));
        Serial.print(test + 1);
        Serial.println(F("/6: Druecke bitte eine Taste."));

        int8_t buttonPin = -1;

        while (buttonPin < 0)
        {
            const int8_t candidate = read_any_mapping_button();

            if (candidate >= 0 &&
                wait_for_stable_button_state(static_cast<uint8_t>(candidate), true))
            {
                buttonPin = candidate;
            }

            delay(10);
        }

        Serial.print(F("PRESS erkannt: "));
        print_mapping_pin(static_cast<uint8_t>(buttonPin));
        Serial.println();

        int8_t functionIndex = -1;

        for (uint8_t i = 0; i < 4; ++i)
        {
            if (detectedButtons[i] == buttonPin)
            {
                functionIndex = static_cast<int8_t>(i);
                break;
            }
        }

        if (functionIndex >= 0 && detectedLeds[functionIndex] >= 0)
        {
            Serial.print(F("Zuordnung: "));
            Serial.println(MAPPING_NAMES[functionIndex]);

            expander.digitalWrite(
                MAPPING_LED_PINS[detectedLeds[functionIndex]], LOW);
        }
        else
        {
            Serial.println(F("FEHLER: Taste nicht in der Zuordnung gefunden."));
        }

        // Explicit release detection before the next test.
        while (!wait_for_stable_button_state(static_cast<uint8_t>(buttonPin), false))
            delay(10);

        Serial.print(F("RELEASE erkannt: "));
        print_mapping_pin(static_cast<uint8_t>(buttonPin));
        Serial.println();

        all_mapping_leds_off();
        delay(200);
    }

    // -------------------------------------------------------------------------
    // Result table
    // -------------------------------------------------------------------------
    Serial.println();
    Serial.println(F("=== MAPPING RESULT ==="));
    Serial.println(F("Funktion        | Taste | LED"));

    for (uint8_t i = 0; i < 4; ++i)
    {
        Serial.print(MAPPING_NAMES[i]);
        Serial.print(F(" | P"));
        Serial.print(detectedButtons[i]);
        Serial.print(F(" | P"));

        if (detectedLeds[i] >= 0)
            Serial.println(MAPPING_LED_PINS[detectedLeds[i]]);
        else
            Serial.println(F("?"));
    }

    Serial.println();
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

    // First line: live state of the four PCF8574 buttons.
    // 1 = pressed, 0 = released.
    display.print(F("G:"));
    display.print(expander.digitalRead(BUTTON_GO_PIN) == LOW ? '1' : '0');
    display.print(F(" S:"));
    display.print(expander.digitalRead(BUTTON_STOP_PIN) == LOW ? '1' : '0');
    display.print(F(" B1:"));
    display.print(expander.digitalRead(BUTTON_LEFT_PIN) == LOW ? '1' : '0');
    display.print(F(" B2:"));
    display.println(expander.digitalRead(BUTTON_RIGHT_PIN) == LOW ? '1' : '0');

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

    display.println(F("HW TEST"));

    display.display();
}


// -----------------------------------------------------------------------------
// Serial command handling
// -----------------------------------------------------------------------------

static void handle_serial_commands()
{
    while (Serial.available() > 0)
    {
        const char command = Serial.read();

        if (command == 'm' || command == 'M')
        {
            // Clear CR/LF and any remaining command characters before entering
            // the blocking test. This prevents the test from being retriggered
            // or skipped by buffered serial input.
            while (Serial.available() > 0)
                Serial.read();

            run_button_led_mapping();

            // Re-synchronise edge detection after the blocking test.
            lastStop  = false;
            lastLeft  = false;
            lastRight = false;
            lastGo    = false;

            // Return directly to loop(). No normal button event is processed
            // on the same pass as the mapping test.
            return;
        }
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

    if (now - tDisplay >= DISPLAY_INTERVAL_MS || hw.displayDirty)
    {
        tDisplay = now;
        update_display();
        hw.displayDirty = false;
    }

    update_outputs();
}
