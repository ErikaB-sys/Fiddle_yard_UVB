#include <Arduino.h>
#include <Wire.h>
#include <freertos/task.h>
#include "FY_Bringup.h"
#include "UART.h"
#include "HTTP.h"
#include "Motor.h"
#include "Keyboard.h"
#include "HW_Config.h"

namespace {
fy::BringupContext bridge{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

// Einmaliger I2C-Hardwaretest vor dem Task-Start.
// Veraendert keine Expander-Ausgaenge und loest keine Motorbefehle aus.
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
    // A4988 waehrend des Starts und der Konfiguration im Schlafzustand halten.
    // !SLEEP und !RESET teilen G32; der externe HW3-Pulldown sichert die
    // hochohmige Startphase. Boot-/Flash-Pegel sind noch nicht separat
    // nachgewiesen (Issue #153).
    digitalWrite(MOTOR_SLEEP_PIN, LOW);
    pinMode(MOTOR_SLEEP_PIN, OUTPUT);
    // Sichere Pegel setzen, bevor die weiteren Pins als Ausgaenge konfiguriert werden.
    digitalWrite(MOTOR_PWM_PIN, LOW);
    digitalWrite(MOTOR_DIR_PIN, LOW);
    digitalWrite(MOTOR_ENABLE_PIN, HIGH); // Treiber deaktiviert (LOW-aktiv).
    pinMode(MOTOR_PWM_PIN, OUTPUT);
    pinMode(MOTOR_DIR_PIN, OUTPUT);
    pinMode(MOTOR_ENABLE_PIN, OUTPUT);
    // Muninn konfiguriert die Mikroschrittleitungen bei deaktiviertem Treiber.
    fy::Muninn_initMotor();
    // Bei POR/Reset keine kuenstlichen STEP-/DIR-Pulse erzeugen.
    // STOP bleibt in Muninn bis zum ausdruecklichen STOP AA 55 verriegelt.
    digitalWrite(MOTOR_PWM_PIN, LOW);
    digitalWrite(MOTOR_DIR_PIN, LOW);
    digitalWrite(MOTOR_ENABLE_PIN, HIGH);

    // Zuerst alle Kommunikationsdatenstrukturen bereitstellen.
    bridge.commands = xQueueCreate(8, sizeof(fy::CommandMessage));
    bridge.events = xQueueCreate(12, sizeof(fy::MotorEvent));
    bridge.priorityCommands = xQueueCreate(4, sizeof(fy::CommandMessage));
    bridge.keyboardEvents = xQueueCreate(8, sizeof(fy::MotorEvent));
    bridge.sensorSnapshots = xQueueCreate(1, sizeof(uint8_t));
    bridge.httpEvents = xQueueCreate(8, sizeof(fy::MotorEvent));
    if (!bridge.commands || !bridge.events || !bridge.priorityCommands || !bridge.keyboardEvents || !bridge.sensorSnapshots || !bridge.httpEvents) {
        Serial.println("FY_TEST: failed to create queues");
        return;
    }

    // Danach I2C-Hardware pruefen und das lokale Keyboard anbinden.
    // Keyboard_begin() darf erst mit vollstaendigem Queue-Kontext laufen.
    scanI2cDevices();
    fy::Keyboard_begin(&bridge);

    // HTTP nur vorbereiten: kein WLAN-Aufbau vor Muninn/Huginn.
    // Eine fehlende Diagnosequeue darf die LOCAL-Firmware nicht verhindern.
    const bool httpPrepared = fy::HTTP_prepare(&bridge);

    // Erst anschliessend die Modul-Tasks auf ihren Kernen starten.
    const BaseType_t muninn = fy::Muninn_startMotor(&bridge);
    const BaseType_t huginn = fy::Huginn_startUART(&bridge);
    if (muninn != pdPASS || huginn != pdPASS) {
        Serial.println("FY_TEST: failed to create tasks");
        return;
    }
    // HTTP bleibt separat; sein Task blockiert weder das Keyboard
    // noch die Kommunikation von Huginn mit Muninn.
    if (httpPrepared && fy::Huginn_startHTTP(&bridge) != pdPASS) {
        Serial.println("FY_HTTP: failed to start HTTP task");
    }
}

void loop() {
    // Keyboard zyklisch abfragen; Fahrbefehle gehen nur ueber die Muninn-Queues.
    fy::Keyboard_poll();
    vTaskDelay(pdMS_TO_TICKS(10));
}
