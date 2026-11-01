/**
 * @file main.cpp
 * @brief FY-ESP32-Web-Pruefstand: WLAN-Bring-up ohne Motorsteuerung.
 *
 * Zugangsdaten stehen ausschliesslich in der lokalen, ignorierten
 * include/WiFi_Secrets.h (Vorlage: WiFi_Secrets.example.h).
 */
#include <Arduino.h>
#include <WiFi.h>
#include "WiFi_Secrets.h"

namespace {
constexpr unsigned long WIFI_TIMEOUT_MS = 15000UL;
constexpr unsigned long WIFI_RETRY_MS = 10000UL;

void connectWiFi()
{
    Serial.println("[FY-WEB] WLAN-Verbindung wird aufgebaut ...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(FY_WIFI_SSID, FY_WIFI_PASSWORD);

    const unsigned long start = millis();
    while ((WiFi.status() != WL_CONNECTED) &&
           ((millis() - start) < WIFI_TIMEOUT_MS)) {
        delay(250);
        Serial.print('.');
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[FY-WEB] WLAN verbunden.");
        Serial.print("[FY-WEB] IP-Adresse: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("[FY-WEB] Keine Verbindung (Timeout).");
    }
}
} // namespace

void setup()
{
    Serial.begin(115200);
    delay(500);
    Serial.println();
    Serial.println("[FY-WEB] ESP32 WLAN-Pruefstand");
    connectWiFi();
}

void loop()
{
    static unsigned long lastRetry = 0UL;

    if (WiFi.status() != WL_CONNECTED) {
        const unsigned long now = millis();
        if ((now - lastRetry) >= WIFI_RETRY_MS) {
            lastRetry = now;
            connectWiFi();
        }
    }
    delay(100);
}
