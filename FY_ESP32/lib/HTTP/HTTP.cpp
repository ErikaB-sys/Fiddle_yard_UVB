#include "HTTP.h"
#include "Motor.h"
#include "Error.h"
#include "CommandRouter.h"
#include "Firmware.h"
#include "Web/FyDiagPage.h"
#include "Web/FyNetworkPage.h"
#include "NetworkSettings.h"
#include "Web/FyWebPage.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <freertos/task.h>

#include <cstdio>
#include <cstring>
#include <atomic>
#include <esp_system.h>

// Test-Fahrbefehle sind NUR im expliziten Entwicklungs-Build erlaubt.
// Keine Bedienhoheits-Umschaltung und keine JMRI-Freigabe in diesem Stand.
#ifndef FY_HTTP_WEB_TEST_CONTROL
#define FY_HTTP_WEB_TEST_CONTROL 0
#endif

// WLAN-Zugangsdaten werden ausschliesslich im Netzwerk-NVS gespeichert.
// Keine WiFi_Secrets.h und keine produktiven Passwoerter im Firmware-Binary.

namespace fy {
namespace {
constexpr char kHostname[] = "fy-uvb";
constexpr uint32_t kWiFiRetryMs = 20000UL;
constexpr uint32_t kSetupApGraceMs = 15UL * 60UL * 1000UL;
constexpr uint32_t kSetupAfterSaveMs = 90000UL; // Zeit fuer Wechsel ins Heim-WLAN
constexpr uint32_t kUnknownPosition = 0x00007FFFUL;
constexpr UBaseType_t kTraceDepth = 16;
constexpr uint32_t kHttpSequenceMask = 0x3FFFFFFFUL;
constexpr uint32_t kHttpSequenceTag = 0x40000000UL;
constexpr uint32_t kMotorAckTimeoutMs = 350UL;

struct TraceLine {
    char line[124];
};

QueueHandle_t traceQueue = nullptr;
BringupContext* httpContext = nullptr;
uint8_t selectedTrack = 3U;
uint32_t selectionRevision = 0U;
uint32_t httpSequence = 0U;
uint32_t httpRxCount = 0U;
uint32_t httpTxCount = 0U;
uint32_t lastRxMs = 0U;
uint32_t lastTxMs = 0U;
const char* lastCommand = "NONE";
const char* lastResult = "NONE";
const char* lastReason = "NONE";
WebServer webServer(80);
constexpr uint32_t kWebLeaseMs = 10000UL;
std::atomic<uint8_t> controlMode{static_cast<uint8_t>(ControlMode::LOCAL)};
String ownerToken;
uint32_t ownerLastSeenMs = 0U;
// Eigentum ausschliesslich beim Huginn-HTTP-Task; keine Shared-State-Zugriffe.
NetworkSettings savedNetwork{};
NetworkSettings pendingNetwork{};
bool pendingNetworkTest = false;
bool pendingNetworkStarted = false;
uint32_t pendingNetworkStartMs = 0U;
bool setupApActive = false;
uint32_t setupApStartedMs = 0U;
char setupApPassword[17]{};
String setupApSsid;
const char* networkResult = "idle";

/** @brief Entzieht eine abgelaufene Web-Bedienberechtigung. */
void expireWebLease() {
    if (controlMode.load() == static_cast<uint8_t>(ControlMode::REMOTE) &&
        static_cast<uint32_t>(millis() - ownerLastSeenMs) >= kWebLeaseMs) {
        ownerToken = "";
        controlMode.store(static_cast<uint8_t>(ControlMode::LOCAL));
    }
}

/** @brief Vergleicht Token serverseitig, nicht IP oder Browserkennung. */
bool isWebOwner() {
    return controlMode.load() == static_cast<uint8_t>(ControlMode::REMOTE) &&
           ownerToken.length() != 0U && webServer.arg("token") == ownerToken;
}

/** @brief Bedienhoheit darf nur nach bestaetigtem Stillstand wechseln. */
bool controlStandstill() {
    const MotorState_t state = Muninn_motorState();
    return !Muninn_isMoving() && state != MotorState_t::MOVING &&
           state != MotorState_t::REFRUN;
}

bool isSetupClient() {
    return setupApActive && webServer.client().localIP() == WiFi.softAPIP();
}

/** @brief Erlaubt Konfiguration auch direkt im privaten Heim-WLAN.
 * @warning Ohne HTTPS/Administrator-Authentifizierung; nur vertrautes LAN.
 */
bool isStationClient() {
    return WiFi.status() == WL_CONNECTED &&
           webServer.client().localIP() == WiFi.localIP();
}

/** @brief Startet ein WPA2-geschuetztes, nur lokal lesbar dokumentiertes Setup-WLAN. */
bool startSetupAp() {
    if (setupApActive) return true;
    char id[7]{};
    std::snprintf(id, sizeof(id), "%06lx",
                  static_cast<unsigned long>(ESP.getEfuseMac() & 0xFFFFFFULL));
    setupApSsid = String("FY-UVB-Setup-") + id;
    std::snprintf(setupApPassword, sizeof(setupApPassword), "%08lx%08lx",
                  static_cast<unsigned long>(esp_random()),
                  static_cast<unsigned long>(esp_random()));
    WiFi.mode(WIFI_AP_STA);
    if (!WiFi.softAP(setupApSsid.c_str(), setupApPassword)) {
        Serial.println(F("[NET] Setup-AP konnte nicht gestartet werden"));
        return false;
    }
    setupApActive = true;
    setupApStartedMs = millis();
    // Fehlerstatus beim automatischen Recovery nicht ueberschreiben.
    Serial.print(F("[NET] Setup-SSID: "));
    Serial.println(setupApSsid);
    Serial.print(F("[NET] Setup-Passwort (nur UART): "));
    Serial.println(setupApPassword);
    Serial.print(F("[NET] Konfiguration: http://"));
    Serial.print(WiFi.softAPIP());
    Serial.println(F("/network"));
    return true;
}

void stopSetupAp() {
    if (!setupApActive) return;
    WiFi.softAPdisconnect(false);
    // STA muss weiterlaufen; softAPdisconnect(true) koennte das Radio abschalten.
    WiFi.mode(WIFI_STA);
    setupApActive = false;
    setupApPassword[0] = '\0';
    Serial.println(F("[NET] Setup-AP beendet"));
}

/** @brief Nur die SSID ausgeben, nie das reale WLAN-Passwort. */
String jsonEscape(const String& value) {
    String out;
    out.reserve(value.length() + 8U);
    for (size_t i = 0U; i < value.length(); ++i) {
        const char c = value[i];
        if (c == '"' || c == '\\') {
            out += '\\';
            out += c;
        } else if (static_cast<uint8_t>(c) < 0x20U) {
            out += '?';
        } else {
            out += c;
        }
    }
    return out;
}

String networkStatusJson() {
    const bool connected = WiFi.status() == WL_CONNECTED;
    String json = F("{\"connected\":");
    json += connected ? F("true") : F("false");
    json += F(",\"ip\":\"");
    json += connected ? WiFi.localIP().toString() : String("0.0.0.0");
    json += F("\",\"ssid\":\"");
    json += jsonEscape(savedNetwork.ssid);
    json += F("\",\"rssi\":");
    json += String(connected ? WiFi.RSSI() : 0);
    json += F(",\"setup_active\":");
    json += setupApActive ? F("true") : F("false");
    json += F(",\"setup_ssid\":\"");
    json += jsonEscape(setupApActive ? setupApSsid : String(""));
    json += F("\",\"setup_client\":");
    json += isSetupClient() ? F("true") : F("false");
    json += F(",\"pending\":");
    json += pendingNetworkTest ? F("true") : F("false");
    json += F(",\"result\":\"");
    json += networkResult;
    json += F("\"}");
    return json;
}


/** @brief Schreibt genau eine HTTP-Kommunikationsmeldung in die Debugqueue.
 * @note Nicht blockierend; eine volle Debugqueue darf den Webserver nie aufhalten.
 */
void trace(const char* dir, const char* cmd, const char* result,
           const char* reason, uint8_t value) {
    if (traceQueue == nullptr) return;
    TraceLine entry{};
    if (std::strcmp(dir, "RX") == 0) {
        ++httpRxCount;
        lastRxMs = millis();
        lastCommand = cmd;
        std::snprintf(entry.line, sizeof(entry.line),
            "{\"transport\":\"HTTP\",\"dir\":\"RX\",\"cmd\":\"%s\",\"value\":%u}",
            cmd, static_cast<unsigned>(value));
    } else {
        ++httpTxCount;
        lastTxMs = millis();
        lastResult = result;
        lastReason = reason;
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
    if (cmd == "UNLOCK") return "UNLOCK";
    if (cmd == "ENABLE") return "ENABLE";
    if (cmd == "DISABLE") return "DISABLE";
    if (cmd == "MODE") return "MODE";
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

/** @brief Benennt einen durch Muninn begruendeten NACK, nie eigenmaechtig ACK. */
const char* motorNackReason(uint8_t responseData) {
    switch (responseData & FY_ERROR_TYPE_MASK) {
    case static_cast<uint8_t>(NackReason::BUSY): return "BUSY";
    case static_cast<uint8_t>(NackReason::NOT_READY): return "NOT_READY";
    case static_cast<uint8_t>(NackReason::INVALID_COMMAND): return "INVALID_COMMAND";
    case static_cast<uint8_t>(NackReason::INVALID_PARAMETER): return "INVALID_PARAMETER";
    case static_cast<uint8_t>(NackReason::EXECUTION_ERROR): return "EXECUTION_ERROR";
    case static_cast<uint8_t>(NackReason::INVALID_TELEGRAM): return "INVALID_TELEGRAM";
    default: return "MOTOR_ERROR";
    }
}

/** @brief Uebergibt exakt einen HTTP-Befehl mit eigener Sequence an Muninn.
 * @return true, wenn Muninn per passender Sequenz ACK sendet.
 * @note Ein Timeout bedeutet unbekanntes Annahmeergebnis, niemals ACK.
 */
bool sendMotorCommand(const char* name, uint8_t id,
                      uint8_t a, uint8_t b, bool priority) {
#if FY_HTTP_WEB_TEST_CONTROL
    if (httpContext == nullptr || httpContext->httpEvents == nullptr) {
        reply(name, 503, "NACK", "NOT_READY");
        return false;
    }
    CommandMessage command{};
    command.id = id;
    command.data[0] = a;
    command.data[1] = b;
    command.length = (id == CMD_REFERENCE || id == CMD_SET_TRACK) ? 2U : 3U;
    httpSequence = (httpSequence % kHttpSequenceMask) + 1U;
    command.sequence = kHttpSequenceTag | httpSequence;

    if (!CommandRouter_enqueue(httpContext, command, priority)) {
        reply(name, 503, "NACK", "QUEUE_BUSY");
        return false;
    }

    MotorEvent event{};
    const TickType_t start = xTaskGetTickCount();
    const TickType_t timeout = pdMS_TO_TICKS(kMotorAckTimeoutMs);
    // Keine zweite Motor-Task: Antwort nur aus der HTTP-eigenen Mailbox.
    // Diese Wartezeit betrifft nur den HTTP-Task, NICHT Muninn oder Keyboard.
    while ((xTaskGetTickCount() - start) < timeout) {
        const TickType_t remaining = timeout - (xTaskGetTickCount() - start);
        if (xQueueReceive(httpContext->httpEvents, &event, remaining) != pdTRUE)
            break;
        if (event.sequence != command.sequence || event.commandId != command.id)
            continue; // Veraltete Antwort eines vorherigen Timeouts.
        if (event.responseId == STATUS_ACK &&
            event.responseData == command.id) {
            reply(name, 202, "ACK", "ACCEPTED_BY_MUNINN");
            return true;
        }
        if (event.responseId == STATUS_NACK) {
            const char* reason = motorNackReason(event.responseData);
            reply(name,
                  (reason == nullptr) ? 500 :
                  (strcmp(reason, "BUSY") == 0 ? 409 :
                  strcmp(reason, "NOT_READY") == 0 ? 423 : 400),
                  "NACK", reason);
            return false;
        }
    }
    // Das Ergebnis kann bei einem Timeout unbekannt sein: kein falsches NACK
    // behaupten und erst nach Statusabfrage erneut senden.
    reply(name, 504, "UNKNOWN", "MOTOR_ACK_TIMEOUT");
    return false;
#else
    (void)id; (void)a; (void)b; (void)priority;
    reply(name, 423, "NACK", "WEB_TEST_DISABLED");
    return false;
#endif
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
    // Erst alle Gleispositionen auf streng monotone Reihenfolge pruefen,
    // danach eine Lage zwischen benachbarten Gleisen interpolieren.
    const bool rising = trackSteps[4] > trackSteps[0];
    for (uint8_t i = 0U; i < 4U; ++i) {
        if (rising ? (trackSteps[i + 1U] <= trackSteps[i]) :
                     (trackSteps[i + 1U] >= trackSteps[i])) return false;
    }
    for (uint8_t i = 0U; i < 4U; ++i) {
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
    json += F("{\"device\":\"FY-UVB\",\"simulation\":false,\"mode\":\"");
    json += controlMode.load() == static_cast<uint8_t>(ControlMode::REMOTE) ? "REMOTE" :
            controlMode.load() == static_cast<uint8_t>(ControlMode::AUTO) ? "AUTO" : "LOCAL";
    json += F("\",\"web_owner\":");
    json += isWebOwner() ? F("true") : F("false");
    json += F(",");
    json += F("\"readonly\":");
    json += (FY_HTTP_WEB_TEST_CONTROL && isWebOwner()) ? F("false") : F("true");
    json += F(",\"web_control\":\"");
    json += isWebOwner() ? F("ACTIVE") : F("LOCKED");
    json += F("\",\"stop_latched\":");
    json += Muninn_isStopLatched() ? F("true") : F("false");
    json += F(",\"driver_enabled\":");
    json += Muninn_isDriverEnabled() ? F("true") : F("false");
    json += F(",\"state\":\"");
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

/** @brief Baut nur lesbare Diagnosedaten aus Snapshots und Queue-Zaehlern. */
String diagnosticsJson() {
    const uint32_t track = Muninn_trackSnapshot();
    const uint16_t nvm = Muninn_nvmStatusSnapshot();
    const uint8_t sensors = Muninn_sensorLevelsSnapshot();
    const bool wifiOnline = WiFi.status() == WL_CONNECTED;

    String motor = statusJson();
    // Gleicher Status wie im Bedienpult, ergaenzt um Diagnosefelder.
    motor.remove(motor.length() - 1U);
    motor += F(",\"confirmed_track\":");
    motor += String((track >> 8U) & 0xFFU);
    motor += F(",\"direction\":");
    motor += String(Muninn_motionDirectionSnapshot());
    motor += F(",\"sensor_raw\":");
    motor += String(sensors);
    motor += F(",\"sensor_levels\":{\"LSL\":");
    motor += String((sensors & 0x01U) ? 1 : 0);
    motor += F(",\"LSR\":");
    motor += String((sensors & 0x02U) ? 1 : 0);
    motor += F(",\"REF\":");
    motor += String((sensors & 0x04U) ? 1 : 0);
    motor += F(",\"BELT\":");
    motor += String((sensors & 0x08U) ? 1 : 0);
    motor += F("}}");

    String json;
    json.reserve(1050);
    json += F("{\"motor\":");
    json += motor;
    json += F(",\"system\":{\"firmware\":\"");
    json += FY_FW_VERSION;
    json += F("\",\"build\":\"");
    json += __DATE__ " " __TIME__;
    json += F("\",\"uptime_ms\":");
    json += String(millis());
    json += F(",\"flash_bytes\":");
    json += String(ESP.getFlashChipSize());
    json += F(",\"heap_free_bytes\":");
    json += String(ESP.getFreeHeap());
    json += F(",\"heap_min_free_bytes\":");
    json += String(ESP.getMinFreeHeap());
    json += F(",\"nvm_faults\":");
    json += String(nvm & 0xFFU);
    json += F(",\"nvm_valid\":");
    json += String(nvm >> 8U);
    json += F("},\"network\":{\"connected\":");
    json += wifiOnline ? F("true") : F("false");
    json += F(",\"ip\":\"");
    json += wifiOnline ? WiFi.localIP().toString() : String("0.0.0.0");
    json += F("\",\"rssi\":");
    json += String(wifiOnline ? WiFi.RSSI() : 0);
    json += F("},\"http\":{\"commands_enabled\":");
    json += FY_HTTP_WEB_TEST_CONTROL ? F("true") : F("false");
    json += F(",\"rx_count\":");
    json += String(httpRxCount);
    json += F(",\"tx_count\":");
    json += String(httpTxCount);
    json += F(",\"last_command\":\"");
    json += lastCommand;
    json += F("\",\"last_result\":\"");
    json += lastResult;
    json += F("\",\"last_reason\":\"");
    json += lastReason;
    json += F("\",\"last_rx_ms\":");
    json += String(lastRxMs);
    json += F(",\"last_tx_ms\":");
    json += String(lastTxMs);
    json += F("},\"queues\":{\"commands\":");
    json += String(httpContext && httpContext->commands ?
                   uxQueueMessagesWaiting(httpContext->commands) : 0U);
    json += F(",\"priority\":");
    json += String(httpContext && httpContext->priorityCommands ?
                   uxQueueMessagesWaiting(httpContext->priorityCommands) : 0U);
    json += F(",\"uart_events\":");
    json += String(httpContext && httpContext->events ?
                   uxQueueMessagesWaiting(httpContext->events) : 0U);
    json += F(",\"keyboard_events\":");
    json += String(httpContext && httpContext->keyboardEvents ?
                   uxQueueMessagesWaiting(httpContext->keyboardEvents) : 0U);
    json += F(",\"http_events\":");
    json += String(httpContext && httpContext->httpEvents ?
                   uxQueueMessagesWaiting(httpContext->httpEvents) : 0U);
    json += F("}}");
    return json;
}

/** @brief Registriert Status, Vorwahl und testweise Muninn-Befehle. */
void configureRoutes() {
    webServer.on("/", HTTP_GET, []() {
        webServer.sendHeader("Cache-Control", "no-store");
        webServer.send_P(200, "text/html; charset=utf-8",
                         isSetupClient() ? FY_NETWORK_PAGE : FY_WEB_PAGE);
    });
    webServer.on("/network", HTTP_GET, []() {
        webServer.sendHeader("Cache-Control", "no-store");
        webServer.send_P(200, "text/html; charset=utf-8", FY_NETWORK_PAGE);
    });
    webServer.on("/api/v1/network/status", HTTP_GET, []() {
        webServer.sendHeader("Cache-Control", "no-store");
        webServer.send(200, "application/json", networkStatusJson());
    });
    webServer.on("/api/v1/network/setup", HTTP_POST, []() {
        if (!controlStandstill() || pendingNetworkTest) {
            webServer.send(409, "text/plain", "BUSY");
            return;
        }
        const bool ok = startSetupAp();
        webServer.send(ok ? 200 : 503, "text/plain",
                       ok ? "SETUP_AP_STARTED" : "SETUP_FAILED");
    });
    webServer.on("/api/v1/network/apply", HTTP_POST, []() {
        // Normaler Wechsel direkt im privaten LAN, Recovery weiterhin per
        // WPA2-Setup-AP. Die Konfiguration laeuft ueber HTTP (ohne TLS).
        if (!isSetupClient() && !isStationClient()) {
            webServer.send(403, "text/plain", "LOCAL_NETWORK_ONLY");
            return;
        }
        if (!controlStandstill() || pendingNetworkTest) {
            webServer.send(409, "text/plain", "BUSY");
            return;
        }
        NetworkSettings candidate{webServer.arg("ssid"), webServer.arg("psk")};
        if (!NetworkSettings_valid(candidate)) {
            webServer.send(400, "text/plain", "INVALID_NETWORK_SETTINGS");
            return;
        }
        pendingNetwork = candidate;
        pendingNetworkTest = true;
        pendingNetworkStarted = false;
        networkResult = "testing";
        webServer.sendHeader("Cache-Control", "no-store");
        webServer.send(202, "text/plain", "NETWORK_TEST_STARTED");
    });
    webServer.on("/api/v1/status", HTTP_GET, []() {
        webServer.sendHeader("Cache-Control", "no-store");
        webServer.send(200, "application/json", statusJson());
    });
    webServer.on("/api/v1/diagnostics", HTTP_GET, []() {
        webServer.sendHeader("Cache-Control", "no-store");
        webServer.send(200, "application/json", diagnosticsJson());
    });
    webServer.on("/diagnostics", HTTP_GET, []() {
        webServer.sendHeader("Cache-Control", "no-store");
        webServer.send_P(200, "text/html; charset=utf-8", FY_DIAG_PAGE);
    });
    webServer.on("/api/v1/mode", HTTP_POST, []() {
        if (isSetupClient() || pendingNetworkTest) {
            webServer.send(423, "text/plain", "NETWORK_SETUP");
            return;
        }
        trace("RX", "MODE", nullptr, nullptr, 0U);
        expireWebLease();
        if (!controlStandstill()) {
            reply("MODE", 409, "NACK", "BUSY");
            return;
        }
        const uint8_t mode = controlMode.load();
        if (mode == static_cast<uint8_t>(ControlMode::AUTO)) {
            reply("MODE", 423, "NACK", "AUTO_ACTIVE");
            return;
        }
        if (mode == static_cast<uint8_t>(ControlMode::REMOTE)) {
            if (!isWebOwner()) {
                reply("MODE", 423, "NACK", "NOT_OWNER");
                return;
            }
            ownerToken = "";
            controlMode.store(static_cast<uint8_t>(ControlMode::LOCAL));
            reply("MODE", 200, "ACK", "LOCAL");
            return;
        }
#if FY_HTTP_WEB_TEST_CONTROL
        // Zwei ESP32-Zufallswerte als browserbezogenes Sitzungstoken.
        char token[17]{};
        std::snprintf(token, sizeof(token), "%08lx%08lx",
                      static_cast<unsigned long>(esp_random()),
                      static_cast<unsigned long>(esp_random()));
        ownerToken = token;
        ownerLastSeenMs = millis();
        controlMode.store(static_cast<uint8_t>(ControlMode::REMOTE));
        trace("TX", "MODE", "ACK", "REMOTE", 0U);
        String json = F("{\"result\":\"ACK\",\"reason\":\"REMOTE\",\"token\":\"");
        json += ownerToken;
        json += F("\"}");
        webServer.sendHeader("Cache-Control", "no-store");
        webServer.send(200, "application/json", json);
#else
        reply("MODE", 423, "NACK", "WEB_TEST_DISABLED");
#endif
    });
    webServer.on("/api/v1/heartbeat", HTTP_POST, []() {
        if (isSetupClient()) {
            webServer.send(403, "text/plain", "SETUP_AP_ONLY");
            return;
        }
        expireWebLease();
        if (!isWebOwner()) {
            webServer.send(423, "application/json",
                           "{\"result\":\"NACK\",\"reason\":\"NOT_OWNER\"}");
            return;
        }
        ownerLastSeenMs = millis();
        webServer.sendHeader("Cache-Control", "no-store");
        webServer.send(200, "application/json", "{\"result\":\"ACK\"}");
    });
    webServer.on("/api/v1/command", HTTP_POST, []() {
        if (isSetupClient() || pendingNetworkTest) {
            webServer.send(423, "text/plain", "NETWORK_SETUP");
            return;
        }
        const String command = webServer.arg("command");
        const char* name = traceCommand(command);
        const String rawValue = webServer.arg("value");
        const uint8_t value = rawValue.length() == 1U &&
            rawValue[0] >= '1' && rawValue[0] <= '5'
            ? static_cast<uint8_t>(rawValue[0] - '0') : 0U;
        trace("RX", name, nullptr, nullptr, value);
        expireWebLease();
        if (!isWebOwner()) {
            reply(name, 423, "NACK", "NOT_OWNER");
            return;
        }
        if (command == "SELECT_TRACK") {
            if (value == 0U) {
                reply(name, 400, "NACK", "INVALID_PARAMETER");
            } else if (Muninn_isMoving() ||
                       Muninn_motorState() == MotorState_t::MOVING ||
                       Muninn_motorState() == MotorState_t::REFRUN) {
                reply(name, 409, "NACK", "BUSY");
            } else {
                // Nur Darstellung/Vorwahl, kein CommandMessage an Muninn!
                if (selectedTrack != value) {
                    selectedTrack = value;
                    ++selectionRevision;
                }
                reply(name, 202, "ACK", "ACCEPTED");
            }
        } else if (command == "SET_TRACK") {
            if (value == 0U) reply(name, 400, "NACK", "INVALID_PARAMETER");
            else (void)sendMotorCommand(name, CMD_SET_TRACK, value, 0U, false);
        } else if (command == "REFERENCE") {
            // Standardmaessig SHORT (00), LONG (01) nur explizit.
            if (rawValue.length() != 0U &&
                rawValue != "0" && rawValue != "1")
                reply(name, 400, "NACK", "INVALID_PARAMETER");
            else
                (void)sendMotorCommand(name, CMD_REFERENCE,
                                       rawValue == "1" ? 1U : 0U, 0U, false);
        } else if (command == "UNLOCK") {
            // Nur verriegelten Stillstand entriegeln; keine laufende Fahrt abbrechen.
            if (!Muninn_isStopLatched())
                reply(name, 409, "NACK", "ALREADY_UNLOCKED");
            else if (Muninn_isMoving() || Muninn_motorState() != MotorState_t::STOPPED)
                reply(name, 409, "NACK", "NOT_STOPPED");
            else
                (void)sendMotorCommand(name, CMD_STOPP, 0xAAU, 0x55U, true);
        } else if (command == "ENABLE") {
            if (Muninn_isStopLatched() || Muninn_isMoving())
                reply(name, 409, "NACK", "NOT_READY");
            else
                (void)sendMotorCommand(name, CMD_GO, 0xAAU, 0x55U, false);
        } else if (command == "DISABLE") {
            if (Muninn_isMoving())
                reply(name, 409, "NACK", "BUSY");
            else
                (void)sendMotorCommand(name, CMD_GO, 0x55U, 0xAAU, false);
        } else {
            reply(name, 400, "NACK", "INVALID_COMMAND");
        }
    });
    webServer.on("/api/v1/stop", HTTP_POST, []() {
        trace("RX", "STOP", nullptr, nullptr, 0U);
        // Die bestehende Muninn-Prioritaetsqueue ist der einzige STOP-Pfad.
        (void)sendMotorCommand("STOP", CMD_STOPP, 0x55U, 0xAAU, true);
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
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(kHostname);
    const bool hasStoredNetwork = NetworkSettings_load(savedNetwork);
    Serial.println(hasStoredNetwork ?
                   F("[NET] WLAN-Daten aus NVS geladen") :
                   F("[NET] Keine gueltigen WLAN-Daten: Einrichtungsmodus"));
    configureRoutes();
    bool listening = false;
    bool mdns = false;
    bool attempted = false;
    bool wasOnline = false;
    uint32_t lastAttempt = 0U;
    uint32_t offlineSince = millis();
    uint32_t connectedSince = 0U;
    if (!hasStoredNetwork) (void)startSetupAp();

    for (;;) {
        const uint32_t now = millis();
        expireWebLease();
        // Kandidat erst verbinden, danach (im Stillstand) ins NVS uebernehmen.
        if (pendingNetworkTest && !pendingNetworkStarted) {
            WiFi.disconnect(false);
            WiFi.begin(pendingNetwork.ssid.c_str(),
                       pendingNetwork.password.c_str());
            pendingNetworkStartMs = now;
            pendingNetworkStarted = true;
            attempted = true;
            lastAttempt = now;
            Serial.println(F("[NET] Teste neue Zugangsdaten ..."));
        }
        const bool online = WiFi.status() == WL_CONNECTED;
        if (online != wasOnline) {
            if (online) {
                connectedSince = now;
                Serial.print(F("[HTTP] WLAN verbunden, IP: "));
                Serial.println(WiFi.localIP());
                Serial.print(F("[HTTP] Web: http://"));
                Serial.print(WiFi.localIP());
                Serial.println('/');
            } else {
                offlineSince = now;
                Serial.println(F("[HTTP] WLAN-Verbindung verloren"));
            }
            wasOnline = online;
        }

        if (pendingNetworkTest && pendingNetworkStarted) {
            if (online && WiFi.SSID() == pendingNetwork.ssid &&
                static_cast<uint32_t>(now - pendingNetworkStartMs) > 750U) {
                // NVS-Schreiben nur im Stillstand; keine Konkurrenz zur Fahrt.
                if (controlStandstill()) {
                    if (NetworkSettings_save(pendingNetwork)) {
                        savedNetwork = pendingNetwork;
                        networkResult = "saved";
                        Serial.println(F("[NET] Neue WLAN-Daten im NVS gespeichert"));
                    } else {
                        networkResult = "storage_error";
                        Serial.println(F("[NET] Fehler beim Speichern der WLAN-Daten"));
                    }
                    pendingNetwork = {};
                    pendingNetworkTest = false;
                    pendingNetworkStarted = false;
                    connectedSince = now; // AP erst nach sichtbarer Bestaetigung beenden.
                }
            } else if (!online &&
                       static_cast<uint32_t>(now - pendingNetworkStartMs) >= kWiFiRetryMs) {
                networkResult = "failed";
                pendingNetwork = {};
                pendingNetworkTest = false;
                pendingNetworkStarted = false;
                attempted = false; // Gespeicherte Zugangsdaten wieder benutzen.
                WiFi.disconnect(false);
                Serial.println(F("[NET] Verbindung fehlgeschlagen; alte Daten beibehalten"));
            }
        } else if (!online && NetworkSettings_valid(savedNetwork) &&
                   (!attempted || static_cast<uint32_t>(now-lastAttempt) >= kWiFiRetryMs)) {
            Serial.println(F("[HTTP] WLAN-Verbindungsversuch ..."));
            WiFi.begin(savedNetwork.ssid.c_str(), savedNetwork.password.c_str());
            attempted = true;
            lastAttempt = now;
        }

        if (!online && !setupApActive &&
            (!NetworkSettings_valid(savedNetwork) ||
             static_cast<uint32_t>(now - offlineSince) >= kWiFiRetryMs)) {
            (void)startSetupAp();
        }
        if (online && setupApActive && !pendingNetworkTest &&
            ((std::strcmp(networkResult, "saved") == 0 &&
              static_cast<uint32_t>(now - connectedSince) >= kSetupAfterSaveMs) ||
             static_cast<uint32_t>(now - setupApStartedMs) >= kSetupApGraceMs)) {
            stopSetupAp();
        }

        if (online && !mdns) {
            mdns = MDNS.begin(kHostname);
            if (mdns) MDNS.addService("http", "tcp", 80);
        } else if (!online && mdns) {
            MDNS.end();
            mdns = false;
        }
        if ((online || setupApActive) && !listening) {
            webServer.begin();
            listening = true;
        } else if (!online && !setupApActive && listening) {
            webServer.stop();
            listening = false;
        }
        if (listening) webServer.handleClient();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
} // namespace

ControlMode HTTP_controlMode() {
    return static_cast<ControlMode>(controlMode.load());
}

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
    return xTaskCreatePinnedToCore(
        httpTask, "Huginn_HTTP", 8192,
        nullptr, 1, nullptr, CORE_HUGINN);
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
