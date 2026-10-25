#pragma once
#include "StepTimer.h"
#include <stdint.h>

namespace fy {

// Physical direction follows Nano's proven coordinate sign:
// awayMotor=true increases position; RIGHT in the NEW entrance view is motor side.
// Does not rewrite motor DIR polarity or swap wired endstops.
class ReferenceRun {
public:
    enum class Stage : uint8_t {
        IDLE, SEEK_MOTOR, SEEK_OPPOSITE, APPROACH,
        FIRST_PASS, BACKOFF, SECOND_PASS, DONE, ERROR
    };
    bool calibrated() const { return _calibrated; }
    bool active() const { return _active; }
    bool successful() const { return _stage == Stage::DONE; }
    bool failed() const { return _stage == Stage::ERROR; }
    Stage stage() const { return _stage; }
    uint32_t position() const { return _position; }
    uint32_t oppositeEnd() const { return _oppositeEnd; }
    uint32_t refLeft() const { return _refLeft; }
    uint32_t refRight() const { return _refRight; }
    uint32_t refLeftReverse() const { return _refLeftReverse; }
    uint32_t refRightReverse() const { return _refRightReverse; }
    uint32_t refCoarse() const { return _coarse; }

    bool begin(bool full) {
        if (_active || (!full && !_calibrated)) return false;
        _full = full;
        _stage = Stage::SEEK_MOTOR;
        _active = true;
        _position = 0U; // Before finding motor-side endstop, coordinate unknown.
        stepTimerReferenceSetPosition(0U);
        if (!launch(false, 120U, false, false)) {
            fail();
            return false;
        }
        return true;
    }

    void cancel() {
        if (_active) stepTimerStop();
        _active = false;
        _stage = Stage::IDLE;
        // Calibration (as distinct from current reference) survives STOP.
    }

    void update() {
        if (!_active) return;
        ReferenceTimerSample s{};
        if (!stepTimerTakeReferenceDone(s)) return;
        _position = s.position;
        switch (_stage) {
        case Stage::SEEK_MOTOR:
            if (s.reason != ReferenceEnd::ENDSTOP) return fail();
            // Nano short reference establishes this position at LSL.
            _position = 1000U;
            stepTimerReferenceSetPosition(_position);
            if (!_full) return finish();
            _stage = Stage::SEEK_OPPOSITE;
            if (!launch(true, 140U, false, true)) fail();
            return;
        case Stage::SEEK_OPPOSITE:
            if (s.reason != ReferenceEnd::ENDSTOP || !s.coarseSeen)
                return fail();
            _oppositeEnd = s.position;
            _coarse = s.coarse;
            // Nano's coarse+1200 approach with right-end clearance.
            {
                uint32_t target = _coarse + 1200U;
                if (target >= _oppositeEnd)
                    target = _oppositeEnd > 100U ? _oppositeEnd - 100U : 0U;
                if (target >= _position || target < 1000U) return fail();
                _stage = Stage::APPROACH;
                if (!launch(false, 140U, false, false,
                            static_cast<uint16_t>(_position - target))) fail();
            }
            return;
        case Stage::APPROACH:
            if (s.reason != ReferenceEnd::STEP_LIMIT) return fail();
            _stage = Stage::FIRST_PASS;
            if (!launch(false, 80U, true, false)) fail();
            return;
        case Stage::FIRST_PASS:
            if (s.reason != ReferenceEnd::REF_EXIT ||
                !s.entrySeen || !s.exitSeen || s.entry <= s.exit)
                return fail();
            _refRight = s.entry;
            _refLeft = s.exit;
            {
                const uint32_t target = _refLeft > 500U ? _refLeft - 500U : 0U;
                if (target >= _position || _position - target > UINT16_MAX)
                    return fail();
                _stage = Stage::BACKOFF;
                if (!launch(false, 80U, false, false,
                            static_cast<uint16_t>(_position - target))) fail();
            }
            return;
        case Stage::BACKOFF:
            if (s.reason != ReferenceEnd::STEP_LIMIT) return fail();
            _stage = Stage::SECOND_PASS;
            if (!launch(true, 80U, true, false)) fail();
            return;
        case Stage::SECOND_PASS:
            if (s.reason != ReferenceEnd::REF_EXIT ||
                !s.entrySeen || !s.exitSeen || s.exit <= s.entry)
                return fail();
            _refLeftReverse = s.entry;
            _refRightReverse = s.exit;
            // Both passes must produce positive flag lengths.
            if (_oppositeEnd <= 1000U || _refRight <= _refLeft)
                return fail();
            _calibrated = true; // RAM only: NVM/CRC not implemented yet.
            return finish();
        default:
            return fail();
        }
    }

private:
    bool launch(bool away, uint16_t hz, bool stopAtExit, bool coarse,
                uint16_t steps = UINT16_MAX) {
        return stepTimerStartReference(away, hz, stopAtExit, coarse, steps);
    }
    void finish() { _active = false; _stage = Stage::DONE; }
    void fail() { stepTimerStop(); _active = false; _stage = Stage::ERROR; }
    Stage _stage = Stage::IDLE;
    bool _active = false;
    bool _full = false;
    bool _calibrated = false;
    uint32_t _position = 0U;
    uint32_t _coarse = 0U;
    uint32_t _oppositeEnd = 0U;
    uint32_t _refLeft = 0U;
    uint32_t _refRight = 0U;
    uint32_t _refLeftReverse = 0U;
    uint32_t _refRightReverse = 0U;
};

} // namespace fy
