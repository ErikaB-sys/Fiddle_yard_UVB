/**
 * @file main.cpp
 * @brief Eigenstaendiger FY-Web-Pruefstand: WLAN, HTTP und mDNS.
 *
 * Keine Motorsteuerung. Lokale Zugangsdaten ausschliesslich in
 * include/WiFi_Secrets.h (nicht versioniert).
 */
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "WiFi_Secrets.h"

namespace {
constexpr unsigned long WIFI_TIMEOUT_MS = 15000UL;
constexpr unsigned long WIFI_RETRY_MS = 10000UL;
constexpr char HOSTNAME[] = "fy-uvb";

WebServer server(80);
bool serverStarted = false;
bool mdnsStarted = false;
unsigned long lastRetry = 0UL;

// Die Darstellung ist bewusst klein und benoetigt keine externe Bibliothek.
const char PAGE[] PROGMEM = R"html(
<!doctype html>
<html lang="de">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>FY UVB - Web-Pruefstand</title>
  <style>
    body{font:16px system-ui,sans-serif;background:#14202a;color:#eaf1f5;
         max-width:680px;margin:48px auto;padding:0 20px}
    main{background:#203341;border-radius:12px;padding:24px}
    h1{margin-top:0} .ok{color:#8fe0ac}
    code{color:#bce3ff} small{color:#bacbd5}
  </style>
</head>
<body>
  <main>
    <h1>FY UVB - Web-Pruefstand</h1>
    <p class="ok">HTTP-Server laeuft.</p>
    <p>Die Motorsteuerung ist hier bewusst nicht angeschlossen.</p>
    <p>Hostname: <code>fy-uvb.local</code></p>
    <p><small>Status: <a href="/api/status" style="color:#bce3ff">/api/status</a><br>Speicherdiagnose: <a href="/api/system" style="color:#bce3ff">/api/system</a></small></p>
  </main>
</body>
</html>
)html";

/** @brief HTTP-Routen registrieren und Server einmalig starten. */
void startHttpServer()
{
    if (serverStarted) {
        return;
    }
    server.on("/", HTTP_GET, []() {
        server.send_P(200, "text/html; charset=utf-8", PAGE);
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
