/**
 * @file main.cpp
 * @brief FY-Web-Pruefstand mit gemeinsamem Simulationszustand ueber HTTP.
 *
 * Keine Motorsteuerung. Lokale Zugangsdaten ausschliesslich in
 * include/WiFi_Secrets.h (nicht versioniert).
 */
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "WiFi_Secrets.h"
#include "FyWebPage.h"
#include "FyMockController.h"

namespace {
constexpr unsigned long WIFI_TIMEOUT_MS = 15000UL;
constexpr unsigned long WIFI_RETRY_MS = 10000UL;
constexpr char HOSTNAME[] = "fy-uvb";

WebServer server(80);
FyMockController mockFY;  // Ein gemeinsamer Mock-Zustand fuer alle Browser
bool serverStarted = false;
bool mdnsStarted = false;
unsigned long lastRetry = 0UL;

/** @brief Simuliertes ACK/NACK auf dem gemeinsamen HTTP-Befehlskanal. */
void replyMock(FyMockController::Result result)
{
    int httpStatus = 202;
    const char *resultName = "ACK";
    const char *reason = "ACCEPTED";
    switch (result) {
        case FyMockController::Result::ACK:
            break;
        case FyMockController::Result::BUSY:
            httpStatus = 409; resultName = "NACK"; reason = "BUSY"; break;
        case FyMockController::Result::NOT_READY:
            httpStatus = 409; resultName = "NACK"; reason = "NOT_READY"; break;
        case FyMockController::Result::INVALID_PARAMETER:
            httpStatus = 400; resultName = "NACK"; reason = "INVALID_PARAMETER"; break;
    }
    String json;
    json.reserve(105);
    json += F("{\"result\":\"");
    json += resultName;
    json += F("\",\"reason\":\"");
    json += reason;
    json += F("\",\"revision\":");
    json += String(mockFY.revision());
    json += F(",\"simulation\":true}");
    server.sendHeader("Cache-Control", "no-store");
    server.send(httpStatus, "application/json", json);
}

/** @brief HTTP-Routen registrieren und Server einmalig starten. */
void startHttpServer()
{
    if (serverStarted) {
        return;
    }
    server.on("/", HTTP_GET, []() {
        server.sendHeader("Cache-Control", "no-store");
        server.send_P(200, "text/html; charset=utf-8", FY_WEB_PAGE);
    });

    // API v1: exakt ein simuliertes FY-Modell pro ESP32.
    // POST-Daten: application/x-www-form-urlencoded (kein JSON-Parser notwendig).
    server.on("/api/v1/status", HTTP_GET, []() {
        server.sendHeader("Cache-Control", "no-store");
        server.send(200, "application/json", mockFY.statusJson(millis()));
    });
    server.on("/api/v1/command", HTTP_POST, []() {
        const String command = server.arg("command");
        if (command == "SET_TRACK") {
            const String value = server.arg("value");
            if (value.length() != 1 || value[0] < '1' || value[0] > '5') {
                replyMock(FyMockController::Result::INVALID_PARAMETER);
                return;
            }
            replyMock(mockFY.setTrack(static_cast<uint8_t>(value[0] - '0'), millis()));
        } else if (command == "REFERENCE") {
            replyMock(mockFY.reference(millis()));
        } else {
            server.send(400, "application/json",
                        "{\"result\":\"NACK\",\"reason\":\"INVALID_COMMAND\",\"simulation\":true}");
        }
    });
    server.on("/api/v1/stop", HTTP_POST, []() {
        mockFY.stop(millis());
        replyMock(FyMockController::Result::ACK);
    });
    server.on("/api/status", HTTP_GET, []() {
        String json = "{\"device\":\"FY-UVB-Web\",\"wifi\":\"connected\",\"ip\":\"";
        json += WiFi.localIP().toString();
        json += "\",\"uptime_ms\":";
        json += String(millis());
        json += "}";
        server.send(200, "application/json", json);
    });
    // Speicherdiagnose: erkannte Flash-Groesse und Laufzeit-Heap, keine Passwoerter.
    server.on("/api/system", HTTP_GET, []() {
        String json;
        json.reserve(240);
        json += "{\"flash_bytes\":";
        json += String(ESP.getFlashChipSize());
        json += ",\"flash_mhz\":";
        json += String(ESP.getFlashChipSpeed() / 1000000UL);
        json += ",\"heap_total_bytes\":";
        json += String(ESP.getHeapSize());
        json += ",\"heap_free_bytes\":";
        json += String(ESP.getFreeHeap());
        json += ",\"heap_min_free_bytes\":";
        json += String(ESP.getMinFreeHeap());
        json += "}";
        server.send(200, "application/json", json);
    });
    server.onNotFound([]() {
        server.send(404, "text/plain; charset=utf-8", "Nicht gefunden");
    });
    server.begin();
    serverStarted = true;
    Serial.println("[FY-WEB] HTTP-Server auf Port 80 gestartet.");
}

/** @brief mDNS starten, sobald die Netzwerkverbindung besteht. */
void startMdns()
{
    if (mdnsStarted) {
        return;
    }
    if (MDNS.begin(HOSTNAME)) {
        MDNS.addService("http", "tcp", 80);
        mdnsStarted = true;
        Serial.println("[FY-WEB] mDNS: http://fy-uvb.local/");
    } else {
        Serial.println("[FY-WEB] mDNS-Start fehlgeschlagen; IP-Adresse verwenden.");
    }
}

/** @brief WLAN verbinden und erreichbare Adressen ausgeben. */
void connectWiFi()
{
    Serial.println("[FY-WEB] WLAN-Verbindung wird aufgebaut ...");
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(HOSTNAME);
    WiFi.begin(FY_WIFI_SSID, FY_WIFI_PASSWORD);

    const unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED &&
           (millis() - start) < WIFI_TIMEOUT_MS) {
        delay(250);
        Serial.print('.');
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[FY-WEB] WLAN verbunden.");
        Serial.print("[FY-WEB] IP-Adresse: ");
        Serial.println(WiFi.localIP());
        startHttpServer();
        startMdns();
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
    Serial.println("[FY-WEB] ESP32 HTTP/mDNS-Pruefstand");
    connectWiFi();
}

void loop()
{
    // Bewegung stets fortschreiben, unabhaengig von HTTP/WLAN-Verfuegbarkeit.
    mockFY.update(millis());
    if (WiFi.status() == WL_CONNECTED) {
        if (!serverStarted) {
            startHttpServer();
        }
        if (!mdnsStarted) {
            startMdns();
        }
        server.handleClient();
    } else {
        if (mdnsStarted) {
            MDNS.end();
            mdnsStarted = false;
        }
        const unsigned long now = millis();
        if (now - lastRetry >= WIFI_RETRY_MS) {
            lastRetry = now;
            connectWiFi();
        }
    }
    delay(2);
}
