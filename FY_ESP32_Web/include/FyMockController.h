#pragma once

/**
 * @file FyMockController.h
 * @brief Gemeinsamer simulierte FY-Zustand fuer alle HTTP-Clients.
 *
 * Nur Web-Pruefstand: KEINE Motor-, GPIO-, NVM- oder JMRI-Steuerung.
 * Der simulierte Zustand lebt im ESP32 (nicht in jedem Browser).
 */
#include <Arduino.h>
#include <math.h>

class FyMockController
{
public:
    enum class Result : uint8_t { ACK, BUSY, NOT_READY, INVALID_PARAMETER };

    /** @brief Simulierte Bewegung ohne blockierendes delay() fortschreiben. */
    void update(uint32_t now)
    {
        if (!busy_) {
            return;
        }
        const uint32_t elapsed = now - startedMs_;
        const float t = (elapsed >= durationMs_)
            ? 1.0f : static_cast<float>(elapsed) / static_cast<float>(durationMs_);
        const float smooth = t * t * (3.0f - 2.0f * t);
        position_ = from_ + (static_cast<float>(target_) - from_) * smooth;
        if (elapsed >= durationMs_) {
            position_ = static_cast<float>(target_);
            busy_ = false;
            if (referencing_) {
                referenced_ = true;
                referencing_ = false;
            }
            stopped_ = false;
            ++revision_;
        }
    }

    /** @brief Gleis nur vorwaehlen; noch keine Fahrt starten. */
    Result selectTrack(uint8_t track, uint32_t now)
    {
        update(now);
        if (track < 1U || track > 5U) {
            return Result::INVALID_PARAMETER;
        }
        if (busy_) {
            return Result::BUSY;
        }
        if (selectedTrack_ != track) {
            selectedTrack_ = track;
            ++revision_;
        }
        return Result::ACK;
    }

    /** @brief SET_TRACK 1..5 simulieren, BUSY und fehlende REF abweisen. */
    Result setTrack(uint8_t track, uint32_t now)
    {
        update(now);
        if (track < 1U || track > 5U) {
            return Result::INVALID_PARAMETER;
        }
        if (busy_) {
            return Result::BUSY;
        }
        if (!referenced_) {
            return Result::NOT_READY;
        }
        // Bei akzeptierter Fahrt die Vorwahl aller Clients mitnehmen.
        selectedTrack_ = track;
        start(track, now, false);
        return Result::ACK;
    }

    /** @brief Nur Demo: Referenzfahrt zur simulierten Grundstellung BG3. */
    Result reference(uint32_t now)
    {
        update(now);
        if (busy_) {
            return Result::BUSY;
        }
        referenced_ = false;
        start(3U, now, true);
        return Result::ACK;
    }

    /** @brief STOP wirkt ohne Wartequeue, Referenz danach ungueltig. */
    void stop(uint32_t now)
    {
        update(now);
        busy_ = false;
        referencing_ = false;
        referenced_ = false;
        stopped_ = true;
        ++revision_;
    }

    /** @brief Einheitlicher HTTP-Snapshot fuer Laptop, Handy und spaeter JMRI. */
    String statusJson(uint32_t now)
    {
        update(now);
        const float nearest = roundf(position_);
        const bool aligned = !busy_ && referenced_ &&
                             fabsf(position_ - nearest) < 0.002f &&
                             nearest >= 1.0f && nearest <= 5.0f;
        const char *state = busy_ ? (referencing_ ? "REFERENCING" : "MOVING")
                           : stopped_ ? "STOPPED"
                           : referenced_ ? "READY" : "UNREFERENCED";
        String result;
        result.reserve(310);
        result += F("{\"device\":\"FY-UVB-Web\",\"simulation\":true,\"mode\":\"DEMO\"");
        result += F(",\"state\":\"");
        result += state;
        result += F("\",\"position\":");
        result += String(position_, 3);
        result += F(",\"track\":");
        if (aligned) {
            result += String(static_cast<unsigned>(nearest));
        } else {
            result += F("null");
        }
        result += F(",\"selected_track\":");
        result += String(selectedTrack_);
        result += F(",\"target_track\":");
        result += String(target_);
        result += F(",\"busy\":");
        result += busy_ ? F("true") : F("false");
        result += F(",\"referenced\":");
        result += referenced_ ? F("true") : F("false");
        result += F(",\"revision\":");
        result += String(revision_);
        result += F(",\"uptime_ms\":");
        result += String(now);
        result += '}';
        return result;
    }

    uint32_t revision() const { return revision_; }

private:
    void start(uint8_t track, uint32_t now, bool isReference)
    {
        from_ = position_;
        target_ = track;
        startedMs_ = now;
        const float distance = fabsf(static_cast<float>(track) - from_);
        // 25 SVG-Einheiten/Gleis * 39 ms (3x langsamer als Ursprungs-Demo).
        durationMs_ = static_cast<uint32_t>(distance * 975.0f);
        if (durationMs_ < 900U) {
            durationMs_ = 900U;
        }
        busy_ = true;
        referencing_ = isReference;
        stopped_ = false;
        ++revision_;
    }

    float position_ = 3.0f;
    float from_ = 3.0f;
    uint8_t target_ = 3U;
    uint8_t selectedTrack_ = 3U;
    bool busy_ = false;
    bool referencing_ = false;
    bool referenced_ = true;  // Nur Testbedingung! Keine reale Referenz.
    bool stopped_ = false;
    uint32_t startedMs_ = 0;
    uint32_t durationMs_ = 900U;
    uint32_t revision_ = 0U;
};
