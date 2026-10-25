#include "UART.h"
#include "Firmware.h"
#include "Motor.h"
#include <freertos/task.h>
#include <cstring>

namespace fy {
namespace {
// Same CRC-8 (polynomial 0x07, initial ID) as the Nano UART.
uint8_t crc8(uint8_t id, const uint8_t* data, size_t length) {
    uint8_t crc = id;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; ++bit)
            crc = (crc & 0x80U) ?
                static_cast<uint8_t>((crc << 1U) ^ 0x07U) :
                static_cast<uint8_t>(crc << 1U);
    }
    return crc;
}

constexpr uint8_t kErrorUnknownCommand = FY_ERROR_LOCATION_UART | 0x03U;
constexpr uint8_t kErrorLength = FY_ERROR_LOCATION_UART | 0x04U;
constexpr uint8_t kErrorInvalidData = FY_ERROR_LOCATION_UART | 0x05U;
constexpr uint8_t kErrorTelegram = FY_ERROR_LOCATION_UART | 0x06U;
constexpr uint8_t kErrorCrc = FY_ERROR_LOCATION_UART | 0x07U;
constexpr uint32_t kPositionUnknown = 0x00007FFFU; // Nano POSITION_UNKNOWN.
constexpr uint8_t kSystemInit = 0x00U; // FY_SystemState_t::INIT (motor not ready).

// Keep the proven Nano protocol table byte-for-byte unchanged. MOVE is an
// explicit ESP32-only extension; legacy LEFT/RIGHT are not repurposed.
constexpr CommandDefinition kMoveDefinition{
    CMD_MOVE, CommandType::EXECUTE, 3U, false
};
const CommandDefinition* lookup(uint8_t id) {
    if (id == CMD_MOVE) return &kMoveDefinition;
    for (size_t i = 0; i < COMMAND_COUNT; ++i)
        if (commandDefinitions[i].id == id) return &commandDefinitions[i];
    return nullptr;
}
} // anonymous namespace

void UART::begin(BringupContext* context) {
    _context = context;
    _errors.begin();
    _babelFish.begin(115200); // ASCII input and local echo retained.
    Serial.println(F("Hello my friend"));
#if defined(FY_MOTOR_BENCH_TEST) && FY_MOTOR_BENCH_TEST
    Serial.println(F("FY ESP32: Huginn ready, Muninn PROFILE BENCH (120..800 Hz)"));
    Serial.println(F("HW3 REF: 01=LONG calibration; 00=SHORT requires RAM calibration"));
    Serial.println(F("HW3 REF: on success proceed to BG3; NO NVS YET; test carefully"));
#else
    Serial.println(F("FY ESP32: Huginn UART ready, motor backend not ready"));
#endif
}

void UART::update() {
    collectSensorSnapshot();
    collectMotorResponses();
    receive();
    collectMotorResponses();
}

void UART::receive() {
    while (Serial.available() > 0) {
        BabelFishCommand_t decoded{};
        const BabelFishResult result =
            _babelFish.process(static_cast<char>(Serial.read()), decoded);
        if (result == BabelFishResult::NONE) continue;
        if (result == BabelFishResult::INVALID) {
            _errors.setError(kErrorTelegram);
            nack(FY_ERROR_LOCATION_UART, NackReason::INVALID_COMMAND);
            sendResponse();
            continue;
        }

        _command = decoded;
        const CommandDefinition* definition = nullptr;
        if (validateCommand(definition)) decodeCommand(*definition);
        sendResponse();
        // Avoid starving motor acknowledgments during a burst of ASCII input.
        collectMotorResponses();
    }
}

bool UART::validateCommand(const CommandDefinition*& definition) {
    definition = lookup(_command.cmd);
    if (definition == nullptr) {
        _errors.setError(kErrorUnknownCommand);
        nack(FY_ERROR_LOCATION_UART, NackReason::INVALID_COMMAND);
        return false;
    }
    if (_command.length == 0U ||
        static_cast<uint8_t>(_command.length - 1U) != definition->dataLength) {
        _errors.setError(kErrorLength);
        nack(FY_ERROR_LOCATION_UART, NackReason::INVALID_PARAMETER);
        return false;
    }
    if (_command.crc != crc8(_command.cmd, _command.data, definition->dataLength)) {
        _errors.setError(kErrorCrc);
        nack(FY_ERROR_LOCATION_UART, NackReason::INVALID_TELEGRAM);
        return false;
    }
    return true;
}

void UART::decodeCommand(const CommandDefinition& definition) {
    switch (definition.type) {
    case CommandType::IMMEDIATE: decodeImmediate(); break;
    case CommandType::EXECUTE:   decodeExecute(); break;
    case CommandType::PRIORITY:  decodePriority(); break;
    default:
        _errors.setError(kErrorUnknownCommand);
        nack(FY_ERROR_LOCATION_UART, NackReason::INVALID_COMMAND);
        break;
    }
}

void UART::decodeImmediate() {
    switch (_command.cmd) {
    case CMD_GET_STATUS:    handleGetStatus(); break;
    case CMD_GET_ERROR:     handleGetError(); break;
    case CMD_GET_POSITION:  handleGetPosition(); break;
    case CMD_GET_TRACK:     handleGetTrack(); break;
    case CMD_HELP:          handleHelp(); break;
    case CMD_GET_FIRMWARE:  handleGetFirmware(); break;
    default: nack(FY_ERROR_LOCATION_UART, NackReason::INVALID_COMMAND); break;
    }
}

void UART::decodeExecute() {
    switch (_command.cmd) {
    case CMD_REFERENCE:
        if (_command.data[0] > 0x01U) {
            nack(FY_ERROR_LOCATION_UART, NackReason::INVALID_PARAMETER);
            return;
        }
        break;
    case CMD_SET_TRACK:
        if (_command.data[0] < 1U || _command.data[0] > 5U) {
            _errors.setError(kErrorInvalidData);
            nack(FY_ERROR_LOCATION_UART, NackReason::INVALID_PARAMETER);
            return;
        }
        break;
    case CMD_GO:
        // Nano GO: 55 AA disables, AA 55 enables the motor driver.
        if (!((_command.data[0] == 0x55U && _command.data[1] == 0xAAU) ||
              (_command.data[0] == 0xAAU && _command.data[1] == 0x55U))) {
            nack(FY_ERROR_LOCATION_MOTOR, NackReason::INVALID_PARAMETER);
            return;
        }
        break;
    case CMD_SET_MODE:
        handleSetMode();
        return;
    case CMD_MOVE: {
        MoveRequest request{};
        // Length and CRC were already validated. Reject reserved flag bits,
        // illegal microstep modes and 0 / >10000 steps before queueing.
        if (!decodeMovePayload(_command.data, request)) {
            _errors.setError(kErrorInvalidData);
            nack(FY_ERROR_LOCATION_MOTOR, NackReason::INVALID_PARAMETER);
            return;
        }
        break;
    }
    case CMD_LEFT:
    case CMD_RIGHT:
        // ESP32 MOVE replaces these for commissioning. Nano files unchanged.
        nack(FY_ERROR_LOCATION_UART, NackReason::INVALID_COMMAND);
        return;
    case CMD_SET_PARAM:
    case CMD_SET_POSITION:
        break;
    default:
        nack(FY_ERROR_LOCATION_UART, NackReason::INVALID_COMMAND);
        return;
    }
    (void)queueMotorCommand(false);
}

void UART::decodePriority() {
    if (_command.cmd != CMD_STOPP) {
        nack(FY_ERROR_LOCATION_UART, NackReason::INVALID_COMMAND);
        return;
    }
    // Nano STOP: 55 AA asserts STOP; AA 55 releases STOP.
    if (!((_command.data[0] == 0x55U && _command.data[1] == 0xAAU) ||
          (_command.data[0] == 0xAAU && _command.data[1] == 0x55U))) {
        nack(FY_ERROR_LOCATION_MOTOR, NackReason::INVALID_PARAMETER);
        return;
    }
    (void)queueMotorCommand(true);
}

bool UART::queueMotorCommand(bool priority) {
    if (_context == nullptr ||
        _context->commands == nullptr ||
        (priority && _context->priorityCommands == nullptr)) {
        nack(FY_ERROR_LOCATION_MOTOR, NackReason::NOT_READY);
        return false;
    }
    CommandMessage message{};
    message.id = _command.cmd;
    message.length = _command.length;
    message.sequence = ++_nextSequence;
    memcpy(message.data, _command.data, sizeof(message.data));

    // Reserve an in-flight slot before handing the request to Muninn.
    PendingCommand* slot = nullptr;
    for (auto& pending : _pending)
        if (!pending.active) { slot = &pending; break; }
    if (slot == nullptr) {
        nack(FY_ERROR_LOCATION_UART, NackReason::BUSY);
        return false;
    }
    // STOP never waits behind the normal motor queue.
    QueueHandle_t queue = priority ? _context->priorityCommands : _context->commands;
    if (xQueueSend(queue, &message, 0) != pdTRUE) {
        nack(FY_ERROR_LOCATION_UART, NackReason::BUSY);
        return false;
    }
    slot->sequence = message.sequence;
    slot->id = message.id;
    slot->active = true;
    // ACK/NACK is emitted only after Muninn reports the result.
    return true;
}

void UART::collectSensorSnapshot() {
    if (_context == nullptr || _context->sensorSnapshots == nullptr) return;
    uint8_t next = 0U;
    while (xQueueReceive(_context->sensorSnapshots, &next, 0) == pdTRUE) {
        _sensorSnapshot = next;
        _sensorSnapshotValid = true;
    }
}

void UART::collectMotorResponses() {
    if (_context == nullptr || _context->events == nullptr) return;
    MotorEvent event{};
    while (xQueueReceive(_context->events, &event, 0) == pdTRUE) {
        // Correlate back to the accepted request before sending any wire reply.
        PendingCommand* slot = nullptr;
        for (auto& pending : _pending) {
            if (pending.active && pending.sequence == event.sequence &&
                pending.id == event.commandId) {
                slot = &pending;
                break;
            }
        }
        if (slot == nullptr) continue; // Stale or unsolicited motor event.
        if (event.responseId != STATUS_ACK && event.responseId != STATUS_NACK)
            continue; // Invalid event: retain slot, do not fabricate a reply.
        if (event.responseId == STATUS_ACK && event.responseData != event.commandId)
            continue;
        slot->active = false;
        setResponse(event.responseId, &event.responseData, 1U);
        sendResponse();
    }
}

void UART::handleGetStatus() {
    switch (_command.data[0]) {
    case STATUS_SELECTOR_SYSTEM: {
        // Nano bit layout: display, expander, UART, motor, buttons, switches,
        // systemError. Only UART has been initialized in this bring-up.
        const uint8_t data[2]{0x04U, kSystemInit};
        setResponse(STATUS_System, data, sizeof(data));
        break;
    }
    case STATUS_SELECTOR_REFERENCE: {
        // Explicitly distinguish real from simulated reference.
        // Real hardware reference is NOT implemented in this branch.
        const uint8_t status = Muninn_referenceStatusSnapshot();
        setResponse(STATUS_Reference, &status, 1U);
        break;
    }
    case STATUS_SELECTOR_MOTOR: {
        // Nano STATUS_Motor DATA[0]: bit 0 = driver enabled,
        // bit 1 = moving. Both snapshots are published by Muninn.
        // No direct GPIO or motor state access from Huginn.
        const uint8_t flags = static_cast<uint8_t>(
            (Muninn_isDriverEnabled() ? 0x01U : 0x00U) |
            (Muninn_isMoving() ? 0x02U : 0x00U));
        // ESP32 port extension: DATA[1] carries MotorState_t for polling.
        // No unsolicited DONE telegram is generated.
        const uint8_t data[3]{flags, static_cast<uint8_t>(Muninn_motorState()), 0U};
        setResponse(STATUS_Motor, data, sizeof(data));
        break;
    }
    case STATUS_SELECTOR_SWITCHES: {
        // Read only the Huginn-owned snapshot, never Muninn GPIOs.
        collectSensorSnapshot();
        if (!_sensorSnapshotValid) {
            nack(FY_ERROR_LOCATION_UART, NackReason::NOT_READY);
            break;
        }
        setResponse(STATUS_Switches, &_sensorSnapshot, 1U);
        break;
    }
    case STATUS_SELECTOR_KEYBOARD: {
        // 0 = not connected (Nano definition).
        const uint8_t data = 0U;
        setResponse(STATUS_Keyboard, &data, 1U);
        break;
    }
    default:
        _errors.setError(kErrorInvalidData);
        nack(FY_ERROR_LOCATION_UART, NackReason::INVALID_PARAMETER);
        break;
    }
}

void UART::handleGetError() {
    const uint8_t data[4]{
        _errors.getSystemByte(),
        _errors.getError(0U),
        _errors.getError(1U),
        _errors.getError(2U)
    };
    setResponse(STATUS_Error, data, sizeof(data));
}

void UART::handleGetPosition() {
    // Nano 4-byte little-endian response. The ESP32-only *bench* flag is
    // bit 16 = 1: lower 16 bits are a simulated position, not an LS reference.
    // If bench tracking is invalid, the original UNKNOWN value is returned.
    const uint32_t value = Muninn_positionSnapshot();
    const uint8_t data[4]{
        static_cast<uint8_t>(value & 0xFFU),
        static_cast<uint8_t>((value >> 8U) & 0xFFU),
        static_cast<uint8_t>((value >> 16U) & 0xFFU),
        static_cast<uint8_t>((value >> 24U) & 0xFFU)
    };
    setResponse(STATUS_Position, data, sizeof(data));
}

void UART::handleGetTrack() {
    if (_command.data[0] != GET_TRACK_OPTION_DEFAULT) {
        _errors.setError(kErrorInvalidData);
        nack(FY_ERROR_LOCATION_UART, NackReason::INVALID_PARAMETER);
        return;
    }
    // Atomically published by Muninn, not inferred from Huginn/GPIO.
    // DATA: requested target, last confirmed track, movement status.
    // Track 00 + ERROR means not referenced/no known track.
    const uint32_t value = Muninn_trackSnapshot();
    const uint8_t data[3]{
        static_cast<uint8_t>(value & 0xFFU),
        static_cast<uint8_t>((value >> 8U) & 0xFFU),
        static_cast<uint8_t>((value >> 16U) & 0xFFU)
    };
    setResponse(STATUS_Track, data, sizeof(data));
}

void UART::handleHelp() { setResponse(STATUS_Help, HELP_RESPONSE); }
void UART::handleGetFirmware() { setResponse(STATUS_Firmware, FW_RESPONSE); }

void UART::handleSetMode() {
    const bool valid =
        (_command.data[0] == 0x55U && _command.data[1] == 0xAAU) ||
        (_command.data[0] == 0xAAU && _command.data[1] == 0x55U);
    if (!valid) {
        nack(FY_ERROR_LOCATION_UART, NackReason::INVALID_PARAMETER);
        return;
    }
    // Nano requires a keyboard/motor context to change the control mode.
    // Do not claim success until both are implemented on ESP32.
    nack(FY_ERROR_LOCATION_UART, NackReason::NOT_READY);
}

void UART::ack(uint8_t cmd) {
    setResponse(STATUS_ACK, &cmd, 1U);
}
void UART::nack(uint8_t location, NackReason reason) {
    const uint8_t data = static_cast<uint8_t>(
        (location & FY_ERROR_LOCATION_MASK) |
        (static_cast<uint8_t>(reason) & FY_ERROR_TYPE_MASK));
    setResponse(STATUS_NACK, &data, 1U);
}

void UART::setResponse(uint8_t id, const uint8_t* data, uint8_t length) {
    if (length > MAX_RESPONSE_LENGTH) length = MAX_RESPONSE_LENGTH;
    _response.id = id;
    _response.length = length;
    _response.container = ResponseContainer::Data;
    _response.string = nullptr;
    if (length != 0U && data != nullptr) memcpy(_response.data, data, length);
    _response.responsePending = true;
}
void UART::setResponse(uint8_t id, const char* text) {
    _response.id = id;
    _response.length = 0U;
    _response.container = ResponseContainer::String;
    _response.string = text;
    _response.responsePending = true;
}

void UART::sendResponse() {
    if (!_response.responsePending) return;
    Serial.write(_response.id);
    if (_response.container == ResponseContainer::String) {
        const size_t length = _response.string ? strlen(_response.string) : 0U;
        for (size_t i = 0; i < length; ++i)
            Serial.write(static_cast<uint8_t>(_response.string[i]));
        Serial.write(crc8(_response.id,
            reinterpret_cast<const uint8_t*>(_response.string), length));
    } else {
        for (uint8_t i = 0; i < _response.length; ++i)
            Serial.write(_response.data[i]);
        Serial.write(crc8(_response.id, _response.data, _response.length));
    }
    _response.responsePending = false;
}

void Huginn_uartTask(void* arg) {
    UART uart;
    uart.begin(static_cast<BringupContext*>(arg));
    for (;;) {
        uart.update();
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

BaseType_t Huginn_startUART(BringupContext* context) {
    return xTaskCreatePinnedToCore(
        Huginn_uartTask, "Huginn_UART", 6144,
        context, 2, nullptr, CORE_HUGINN);
}
} // namespace fy
