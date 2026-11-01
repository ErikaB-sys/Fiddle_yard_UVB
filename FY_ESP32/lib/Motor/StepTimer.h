#pragma once
#include <Arduino.h>

namespace fy {

// Sechs Nano-Phasen ohne Abhaengigkeit von AVR-Timerregistern.
// accel: vorzeichenbehaftete Aenderung in Nano-Ticks (+ bedeutet langsamer).
// rampEvery: Aenderung nach jeweils so vielen abgeschlossenen STEP-Pulsen.
struct StepProfileSegment {
    uint16_t steps;
    int16_t accel;
    uint16_t rampEvery;
};
constexpr uint8_t STEP_PROFILE_PHASES = 6U;

// Nur Muninn startet und stoppt Fahrten. Die ISR zaehlt abgeschlossene
// Pulse an der fallenden Flanke und schaltet dort die Phasen weiter.
bool stepTimerBegin();

// Geschwindigkeiten ausschliesslich im Stillstand konfigurieren.
// ISR liest weiterhin nur ihre bisherigen Reload-Grenzen.
bool stepTimerConfigureProfile(uint16_t vMin, uint16_t vMax, uint16_t vPos);

// Diagnosefahrt mit fester Frequenz; aus Kompatibilitaetsgruenden beibehalten.
bool stepTimerStart(uint16_t steps, bool directionRight, uint16_t stepHz);

// Ausfuehrung des sechsstufigen Nano-Profils. false bei nicht unterstuetztem
// Timerpfad, unpassender Schrittsumme, belegtem Timer, Nullweg oder Rampenfehler.
// Unter Arduino-ESP32 2.x schreibt die ISR das naechste TIMG1/T0-Intervall
// direkt und IRAM-sicher, ohne timerAlarmWrite() in der ISR.
bool stepTimerStartProfile(uint16_t steps, bool directionRight,
                           const StepProfileSegment (&segments)[STEP_PROFILE_PHASES],
                           uint16_t startHz);
// Sensorueberwachte Referenzfahrt mit fester Geschwindigkeit.
// Nano-Richtung: awayMotor=true erhoeht die Position (G39-Seite).
enum class ReferenceEnd : uint8_t { NONE, ENDSTOP, REF_EXIT, STEP_LIMIT };
struct ReferenceTimerSample {
    uint32_t position = 0U;
    uint32_t coarse = 0U;
    uint32_t entry = 0U;
    uint32_t exit = 0U;
    ReferenceEnd reason = ReferenceEnd::NONE;
    bool coarseSeen = false;
    bool entrySeen = false;
    bool exitSeen = false;
};
// Positionsgefuehrte Abschlussfahrt nach BG3 mit sechsstufigem Profil.
// Endschalterpruefung bei jedem STEP; Abschlussmeldung STEP_LIMIT.
bool stepTimerStartReferenceProfile(uint16_t steps, bool awayMotor,
                           const StepProfileSegment (&segments)[STEP_PROFILE_PHASES],
                           uint16_t startHz);
void stepTimerReferenceSetPosition(uint32_t position);
bool stepTimerStartReference(bool awayMotor, uint16_t stepHz,
                             bool stopOnRefExit, bool captureCoarse,
                             uint16_t steps = UINT16_MAX);
bool stepTimerTakeReferenceDone(ReferenceTimerSample& sample);
ReferenceTimerSample stepTimerReferenceSnapshot();

void stepTimerStop();
bool stepTimerRunning();
uint32_t stepTimerCompletedSteps();
// endstopHit meldet einen unerwarteten Endschalter bei normaler Fahrt.
bool stepTimerTakeDone(uint32_t& completedSteps, bool& endstopHit);

} // namespace fy
