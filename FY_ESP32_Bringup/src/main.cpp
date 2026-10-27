#include <Arduino.h>
#include <Wire.h>
#include <freertos/task.h>
#include "FY_Bringup.h"
#include "UART.h"
#include "Motor.h"
#include "Keyboard.h"
#include "HW_Config.h"

namespace {
fy::BringupContext bridge{nullptr, nullptr, nullptr, nullptr, nullptr};

// Einmaliger I2C-Integrationstest vor dem Start der Aufgaben.
// Keine Ausgänge des Expanders verändern und keine Motorbefehle auslösen.
void scanI2cDevices() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 100000U);
    Wire.setTimeOut(50U);
    Serial.println(F("I2C: scan begin (SDA=21 SCL=22, 100 kHz)"));
    uint8_t found = 0U;
    for (uint8_t address = 0x08U; address <= 0x77U; ++address) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0U) {
            Serial.print(F("I2C: found 0x"));
            if (address < 0x10U) Serial.print('0');
            Serial.println(address, HEX);
            ++found;
        }
    }
    Serial.print(F("I2C: scan done, devices="));
    Serial.println(found);
}


} // anonymous namespace

void setup() {
    Serial.begin(115200);
    // Keep the A4988 asleep during boot/configuration. !SLEEP and !RESET
    // share G32; the external HW3 pulldown covers the high-impedance period.
    // Boot/flash pin levels are NOT independently proven yet (Issue #153).
    digitalWrite(MOTOR_SLEEP_PIN, LOW);
    pinMode(MOTOR_SLEEP_PIN, OUTPUT);
    // Safe output levels before configuring the other pins as outputs.
    digitalWrite(MOTOR_PWM_PIN, LOW);
    digitalWrite(MOTOR_DIR_PIN, LOW);
    digitalWrite(MOTOR_ENABLE_PIN, HIGH); // Driver disabled (active LOW).
    pinMode(MOTOR_PWM_PIN, OUTPUT);
    pinMode(MOTOR_DIR_PIN, OUTPUT);
    pinMode(MOTOR_ENABLE_PIN, OUTPUT);
    // Muninn owns the microstep lines, configured while EN is OFF.
    fy::Muninn_initMotorMock();
    // Never generate artificial STEP/DIR pulses on power-on/reset.
    // STOP starts latched in Muninn until an explicit STOP AA 55.
    digitalWrite(MOTOR_PWM_PIN, LOW);
    digitalWrite(MOTOR_DIR_PIN, LOW);
    digitalWrite(MOTOR_ENABLE_PIN, HIGH);

    scanI2cDevices();
    fy::Keyboard_begin(&bridge);

    bridge.commands = xQueueCreate(8, sizeof(fy::CommandMessage));
    bridge.events = xQueueCreate(12, sizeof(fy::MotorEvent));
    bridge.priorityCommands = xQueueCreate(4, sizeof(fy::CommandMessage));
    bridge.keyboardEvents = xQueueCreate(8, sizeof(fy::MotorEvent));
    bridge.sensorSnapshots = xQueueCreate(1, sizeof(uint8_t));
    if (!bridge.commands || !bridge.events || !bridge.priorityCommands || !bridge.keyboardEvents || !bridge.sensorSnapshots) {
        Serial.println("FY_TEST: failed to create queues");
        return;
    }

    // Module owners create their own FreeRTOS tasks on assigned cores.
    const BaseType_t muninn = fy::Muninn_startMotorMock(&bridge);
    const BaseType_t huginn = fy::Huginn_startUART(&bridge);
    if (muninn != pdPASS || huginn != pdPASS) {
        Serial.println("FY_TEST: failed to create tasks");
    }
}

void loop() {
    // Vorübergehender I2C-Hardwaretest; ohne Verbindung zur Motorsteuerung.
    fy::Keyboard_poll();
    vTaskDelay(pdMS_TO_TICKS(10));
}
