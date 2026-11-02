#include "HTTP.h"
#include "Motor.h"
#include "Web/FyWebPage.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <freertos/task.h>
#include <math.h>
#include <cstdio>
#include <cstring>

// Die Zugangsdaten sind absichtlich lokal, nie im Repository.
// Ohne Datei bleibt die funktionierende LOCAL-Firmware baubar und betriebsfaehig.
#if __has_include("WiFi_Secrets.h")
#include "WiFi_Secrets.h"
#define FY_HTTP_WIFI_CONFIGURED 1
#else
#define FY_HTTP_WIFI_CONFIGURED 0
#endif

namespace fy {
namespace {
constexpr char kHostname[] = "fy-uvb";
constexpr uint32_t kWiFiRetryMs = 10000UL;
constexpr uint32_t kUnknownPosition = 0x00007FFFUL;
constexpr UBaseType_t kTraceDepth = 16;

struct TraceLine {
    char line[124];
};

QueueHandle_t traceQueue = nullptr;
BringupContext* httpContext = nullptr;
uint8_t selectedTrack = 3U;
uint32_t selectionRevision = 0U;
WebServer webServer(80);

/** @brief Schreibt genau eine HTTP-Kommunikationsmeldung in die Debugqueue.
 * @note Nicht blockierend; eine volle Debugqueue darf den Webserver nie aufhalten.
 */
void trace(const char* dir, const char* cmd, const char* result,
           const char* reason, uint8_t value) {
    if (traceQueue == nullptr) return;
    TraceLine entry{};
    if (std::strcmp(dir, "RX") == 0) {
        std::snprintf(entry.line, sizeof(entry.line),
            "{\"transport\":\"HTTP\",\"dir\":\"RX\",\"cmd\":\"%s\",\"value\":%u}",
            cmd, static_cast<unsigned>(value));
    } else {
        std::snprintf(entry.line, sizeof(entry.line),
            "{\"transport\":\"HTTP\",\"dir\":\"TX\",\"cmd\":\"%s\","
            "\"result\":\"%s\",\"reason\":\"%s\"}",
            cmd, result, reason);
    }
    (void)xQueueSend(traceQueue, &entry, 0);
}

/** @brief Definierte Befehlsnamen fuer den Trace, nie unkontrollierte URL-Daten. */
const char* traceCommand(const String& cmd) {
    if (cmd == "SELECT_TRACK") return "SELECT_TRACK";
    if (cmd == "SET_TRACK") return "SET_TRACK";
    if (cmd == "REFERENCE") return "REFERENCE";
    if (cmd == "STOP") return "STOP";
    return "UNKNOWN";
}

/** @brief Antwortet mit explizitem ACK/NACK, ohne einen Motorauftrag zu erfinden. */
void reply(const char* cmd, int httpStatus, const char* result,
           const char* reason) {
    trace("TX", cmd, result, reason, 0U);
    String json;
    json.reserve(112);
    json += F("{\"result\":\"");
    json += result;
    json += F("\",\"reason\":\"");
    json += reason;
    json += F("\",\"revision\":");
    json += String(selectionRevision);
    json += F(",\"simulation\":false}");
    webServer.sendHeader("Cache-Control", "no-store");
    webServer.send(httpStatus, "application/json", json);
}

/** @brief Reale Schrittlage auf die grafische BG-Skala 1..5 abbilden.
 * @return false bei fehlender REF, ungueltiger Tracktabelle oder unbekannter Lage.
 */
bool graphicalPosition(uint32_t steps, float& out) {
    if (steps == kUnknownPosition) return false;
    uint32_t trackSteps[5]{};
    for (uint8_t i = 0U; i < 5U; ++i) {
        if (!Muninn_getParamSnapshot(
                static_cast<uint8_t>(ParamId::TRACK_BG1) + i,
                trackSteps[i])) return false;
    }
    // Trackpositionen muessen in einer Richtung streng monoton sein.
    const bool rising = trackSteps[4] > trackSteps[0];
    for (uint8_t i = 0U; i < 4U; ++i) {
        if (rising ? (trackSteps[i + 1U] <= trackSteps[i]) :
                     (trackSteps[i + 1U] >= trackSteps[i])) return false;
        const uint32_t lo = trackSteps[i] < trackSteps[i + 1U]
            ? trackSteps[i] : trackSteps[i + 1U];
        const uint32_t hi = trackSteps[i] > trackSteps[i + 1U]
            ? trackSteps[i] : trackSteps[i + 1U];
        if (steps >= lo && steps <= hi) {
            const float fraction =
                (static_cast<float>(steps) - static_cast<float>(trackSteps[i])) /
                (static_cast<float>(trackSteps[i + 1U]) -
                 static_cast<float>(trackSteps[i]));
            out = static_cast<float>(i + 1U) + fraction;
            return true;
        }
    }
    return false;
}

/** @brief Liest ausschliesslich die von Muninn publizierten Status-Snapshots. */
String statusJson() {
    const MotorState_t state = Muninn_motorState();
    const bool busy = Muninn_isMoving() ||
        state == MotorState_t::REFRUN || state == MotorState_t::MOVING;
    const bool referenced = Muninn_referenceStatusSnapshot() == REFERENCE_REAL;
    const uint32_t packed = Muninn_trackSnapshot();
    const uint8_t target = static_cast<uint8_t>(packed & 0xFFU);
    const uint8_t confirmed = static_cast<uint8_t>((packed >> 8U) & 0xFFU);
    const uint8_t trackState = static_cast<uint8_t>((packed >> 16U) & 0xFFU);
    const uint32_t steps = Muninn_positionSnapshot();
    const bool aligned = !busy && referenced &&
        trackState == TRACK_STATUS_REACHED && confirmed >= 1U && confirmed <= 5U;
    float graphic = 0.0f;
    const bool positionValid = referenced && graphicalPosition(steps, graphic);
    const char* status = state == MotorState_t::REFRUN ? "REFERENCING" :
                         busy ? "MOVING" :
                         state == MotorState_t::STOPPED ? "STOPPED" :
                         state == MotorState_t::ERROR ? "ERROR" :
                         referenced ? "READY" : "UNREFERENCED";
    String json;
    json.reserve(340);
    json += F("{\"device\":\"FY-UVB\",\"simulation\":false,\"mode\":\"LOCAL\",");
    json += F("\"readonly\":true,\"state\":\"");
    json += status;
    json += F("\",\"position\":");
    if (positionValid) json += String(graphic, 3);
    else json += F("null");
    json += F(",\"position_steps\":");
    if (referenced && steps != kUnknownPosition) json += String(steps);
    else json += F("null");
    json += F(",\"track\":");
    if (aligned) json += String(confirmed);
    else json += F("null");
    json += F(",\"target_track\":");
    json += String(target);
    json += F(",\"selected_track\":");
    json += String(selectedTrack);
    json += F(",\"busy\":");
    json += busy ? F("true") : F("false");
    json += F(",\"referenced\":");
    json += referenced ? F("true") : F("false");
    json += F(",\"revision\":");
    json += String(selectionRevision);
    json += F(",\"uptime_ms\":");
    json += String(millis());
    json += '}';
    return json;
}

/** @brief Registriert nur Status und Vorwahl; Motorbefehle sind GESPERRT. */
void configureRoutes() {
    webServer.on("/", HTTP_GET, []() {
        webServer.sendHeader("Cache-Control", "no-store");
        webServer.send_P(200, "text/html; charset=utf-8", FY_WEB_PAGE);
    });
    webServer.on("/api/v1/status", HTTP_GET, []() {
        webServer.sendHeader("Cache-Control", "no-store");
        webServer.send(200, "application/json", statusJson());
    });
    webServer.on("/api/v1/command", HTTP_POST, []() {
        const String command = webServer.arg("command");
        const char* name = traceCommand(command);
        const String rawValue = webServer.arg("value");
        const uint8_t value = rawValue.length() == 1U &&
            rawValue[0] >= '1' && rawValue[0] <= '5'
            ? static_cast<uint8_t>(rawValue[0] - '0') : 0U;
        trace("RX", name, nullptr, nullptr, value);
        if (command == "SELECT_TRACK") {
            if (value == 0U) {
                reply(name, 400, "NACK", "INVALID_PARAMETER");
            } else if (Muninn_isMoving()) {
                reply(name, 409, "NACK", "BUSY");
            } else {
                // Nur Darstellung/Vorwahl, kein CommandMessage an Muninn!
                if (selectedTrack != value) {
                    selectedTrack = value;
                    ++selectionRevision;
                }
                reply(name, 202, "ACK", "ACCEPTED");
            }
        } else {
            // LOCAL-Betrieb ohne Bedienhoheit: keinerlei reale Motorbefehle.
            reply(name, 423, "NACK", "HTTP_CONTROL_LOCKED");
        }
    });
    webServer.on("/api/v1/stop", HTTP_POST, []() {
        trace("RX", "STOP", nullptr, nullptr, 0U);
        // Auch STOP noch nicht als HTTP-Ausloeser freigegeben.
        // Das lokale Keyboard und die bestehende UART-Prioritaetsqueue bleiben wirksam.
        reply("STOP", 423, "NACK", "HTTP_CONTROL_LOCKED");
    });
    webServer.on("/api/status", HTTP_GET, []() {
        String json = F("{\"device\":\"FY-UVB\",\"wifi\":\"connected\",\"ip\":\"");
        json += WiFi.localIP().toString();
        json += F("\",\"uptime_ms\":");
        json += String(millis());
        json += '}';
        webServer.send(200, "application/json", json);
    });
    webServer.on("/api/system", HTTP_GET, []() {
        String json;
        json.reserve(225);
        json += F("{\"flash_bytes\":");
        json += String(ESP.getFlashChipSize());
        json += F(",\"heap_free_bytes\":");
        json += String(ESP.getFreeHeap());
        json += F(",\"heap_min_free_bytes\":");
        json += String(ESP.getMinFreeHeap());
        json += '}';
        webServer.send(200, "application/json", json);
    });
    webServer.onNotFound([]() {
        webServer.send(404, "text/plain; charset=utf-8", "Nicht gefunden");
    });
}

/** @brief WLAN-Verbindung und Server in eigenem Core-0-Task pflegen. */
void httpTask(void*) {
#if FY_HTTP_WIFI_CONFIGURED
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(kHostname);
    configureRoutes();
    bool joined = false;
    bool listening = false;
    bool mdns = false;
    bool attempted = false;
    uint32_t lastAttempt = 0U;
    for (;;) {
        const uint32_t now = millis();
        const bool online = WiFi.status() == WL_CONNECTED;
        if (online) {
            if (!listening) {
                webServer.begin();
                listening = true;
            }
            if (!mdns) {
                mdns = MDNS.begin(kHostname);
                if (mdns) MDNS.addService("http", "tcp", 80);
            }
            joined = true;
            webServer.handleClient();
        } else {
            if (mdns) {
                MDNS.end();
                mdns = false;
            }
            if (listening) {
                webServer.stop();
                listening = false;
            }
            if (!attempted || now - lastAttempt >= kWiFiRetryMs) {
                // WiFi.begin() kehrt zurueck; keine 15-s-Startwarteschleife.
                WiFi.begin(FY_WIFI_SSID, FY_WIFI_PASSWORD);
                attempted = true;
                lastAttempt = now;
            }
            joined = false;
        }
        (void)joined;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
#else
    vTaskDelete(nullptr);
#endif
}
} // namespace

bool HTTP_prepare(BringupContext* context) {
    if (context == nullptr || context->commands == nullptr ||
        context->priorityCommands == nullptr) return false;
    httpContext = context;
    if (traceQueue == nullptr)
        traceQueue = xQueueCreate(kTraceDepth, sizeof(TraceLine));
    return traceQueue != nullptr;
}

BaseType_t Huginn_startHTTP(BringupContext* context) {
    if (context == nullptr || context != httpContext || traceQueue == nullptr)
        return pdFAIL;
#if FY_HTTP_WIFI_CONFIGURED
    return xTaskCreatePinnedToCore(
        httpTask, "Huginn_HTTP", 8192,
        nullptr, 1, nullptr, CORE_HUGINN);
#else
    // Keine Zugangsdaten vorhanden: lokaler Controller laeuft weiter.
    return pdPASS;
#endif
}

void HTTP_flushTrace() {
    if (traceQueue == nullptr) return;
    TraceLine msg{};
    // Begrenzte Ausgabe pro Aufruf: die serielle Verarbeitung bleibt responsiv.
    for (uint8_t i = 0U; i < 4U; ++i) {
        if (xQueueReceive(traceQueue, &msg, 0) != pdTRUE) break;
        Serial.println(msg.line);
    }
}
} // namespace fy
