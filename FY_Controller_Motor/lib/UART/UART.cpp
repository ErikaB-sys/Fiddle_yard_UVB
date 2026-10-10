#include <Arduino.h>
#include <HardwareSerial.h>
#include "FY_System.h"
#include "Protokoll.h"
#include "UART.h"
#include "Error.h"
#include "Firmware.h"
#include "Motor.h"
#include "Switches.h"
#include "Keyboard.h"

UART::UART()
{
    UART_context = nullptr;
    FY_ModuleContext = nullptr;

    CommandBuffer = {};
    CommandBuffer.status = UART_CommandStatus_t::VALID;

    ResponseBuffer = {};
}

void UART::begin(UART_Context_t& context, FY_ModuleContext_t& modules)
{
    Serial.begin(UART_BAUD_RATE);

    UART_context = &context;
    FY_ModuleContext = &modules;

    clearError();
    _babelFish.begin();

    sendHello();
}

void UART::update()
{
    if (UART_context == nullptr)
    {
        setError(UART_ERROR_NO_CONTENT);
        Serial.println(F("No content. The UART is feeling lonely."));
        return;
    }

    if (ResponseBuffer.responsePending)
    {
        sendResponse();
        return;
    }

    // The command mailbox is locked until the current command has
    // been completely processed.
    if (commandReady)
        return;

    receive();

    if (!commandReady)
        return;

    decodeCommand();

    commandReady = false;
}

/**
 * @brief Receives ASCII input through the UART and lets BabelFish
 *        decode it directly into the UART command buffer.
 */
void UART::receive()
{
    while (Serial.available() && !commandReady)
    {
        const char c = static_cast<char>(Serial.read());

        const BabelFishResult result =
            _babelFish.process(c, CommandBuffer.command);

        if (result == BabelFishResult::COMMAND_READY)
        {
            CommandBuffer.status = UART_CommandStatus_t::VALID;

            // CommandBuffer.command.crc ^= 0x01;   // TEST ONLY: corrupt CRC

            if (!validateCommand())
            {
                switch (CommandBuffer.status)
                {
                case UART_CommandStatus_t::CMD_INVALID:
                    setError(UART_ERROR_UNKNOWN_COMMAND);
                    break;
                case UART_CommandStatus_t::DATA_INVALID:
                    setError(UART_ERROR_INVALID_LENGTH);
                    break;
                case UART_CommandStatus_t::CRC_INVALID:
                    setError(UART_ERROR_CRC);
                    break;
                case UART_CommandStatus_t::VALID:
                default:
                    setError(UART_ERROR_INVALID_TELEGRAM);
                    break;
                }

                const uint8_t reason =
                    static_cast<uint8_t>(CommandBuffer.status);

                setResponse(STATUS_NACK, &reason, 1);
                commandReady = false;
                return;
            }

            commandReady = true;
        }
        else if (result == BabelFishResult::INVALID)
        {
            CommandBuffer.status = UART_CommandStatus_t::CMD_INVALID;
            setError(UART_ERROR_INVALID_TELEGRAM);

            const uint8_t reason =
                static_cast<uint8_t>(CommandBuffer.status);

            setResponse(STATUS_NACK, &reason, 1);
            commandReady = false;
            return;
        }
    }
}

/**
 * @brief Validates the received command against the UART protocol.
 *
 * Validation order is deliberately uniform:
 * 1. Command ID
 * 2. Telegram length
 * 3. CRC
 *
 * Command classification is assigned only after all validation checks pass.
 */
bool UART::validateCommand()
{
    for (uint8_t i = 0; i < COMMAND_COUNT; ++i)
    {
        if (commandDefinitions[i].id != CommandBuffer.command.cmd)
            continue;

        if (commandDefinitions[i].dataLength != static_cast<uint8_t>(CommandBuffer.command.length - 1))
        {
            CommandBuffer.status = UART_CommandStatus_t::DATA_INVALID;
            return false;
        }

        const uint8_t calculatedCrc = Calc_CRC(
            CommandBuffer.command.cmd,
            CommandBuffer.command.data,
            static_cast<uint8_t>(CommandBuffer.command.length - 1)
        );

        if (calculatedCrc != CommandBuffer.command.crc)
        {
            CommandBuffer.status = UART_CommandStatus_t::CRC_INVALID;
            return false;
        }

        CommandBuffer.type = commandDefinitions[i].type;
        return true;
    }

    CommandBuffer.status = UART_CommandStatus_t::CMD_INVALID;
    return false;
}

void UART::decodeCommand()
{
    switch (CommandBuffer.type)
    {
    case CommandType::IMMEDIATE:
        decodeImmediate();
        break;

    case CommandType::EXECUTE:
        decodeExecute();
        break;

    case CommandType::PRIORITY:
        decodePriority();
        break;
    }
}

void UART::decodeImmediate()
{
    switch (CommandBuffer.command.cmd)
    {
    case CMD_GET_STATUS:
        handleGetStatus();
        break;

    case CMD_GET_ERROR:
        handleGetError();
        break;

    case CMD_GET_POSITION:
        handleGetPosition();
        break;

    case CMD_GET_TRACK:
        handleGetTrack();
        break;

    case CMD_HELP:
        handleHelp();
        break;

    case CMD_GET_FIRMWARE:
        handleGetFirmware();
        break;

    default:
        CommandBuffer.status = UART_CommandStatus_t::CMD_INVALID;
        break;
    }
}

void UART::decodeExecute()
{
    if (UART_context->systemStatus->state == FY_SystemState_t::Busy)
    {
        handle_Busy();
        return;
    }

    switch (CommandBuffer.command.cmd)
    {
    case CMD_REFERENCE:
        handleReference();
        break;

    case CMD_SET_SPEED:
        handleSetSpeed();
        break;

    case CMD_GO:
        handleGo();
        break;

    case CMD_LEFT:
        handleLeft();
        break;

    case CMD_RIGHT:
        handleRight();
        break;

    case CMD_SET_POSITION:
        handleSetPosition();
        break;

    case CMD_SET_TRACK:
        handleSetTrack();
        break;

    case CMD_SET_MODE:
        handleSetMode();
        break;

    default:
        CommandBuffer.status = UART_CommandStatus_t::CMD_INVALID;
        break;
    }
}

void UART::decodePriority()
{
    switch (CommandBuffer.command.cmd)
    {
    case CMD_STOPP:
        handleStop();
        break;

    default:
        CommandBuffer.status = UART_CommandStatus_t::CMD_INVALID;
        break;
    }
}

void UART::sendResponse()
{
    if (!ResponseBuffer.responsePending)
        return;

    Serial.write(ResponseBuffer.id);

    if (ResponseBuffer.container == ResponseContainer::String)
    {
        const uint8_t length = static_cast<uint8_t>(strlen_P(ResponseBuffer.string));

        for (uint8_t i = 0; i < length; ++i)
            Serial.write(pgm_read_byte(&ResponseBuffer.string[i]));

        const uint8_t crc = Calc_CRC_PGM(
            ResponseBuffer.id,
            ResponseBuffer.string,
            length
        );

        Serial.write(crc);
    }
    else
    {
        for (uint8_t i = 0; i < ResponseBuffer.length; ++i)
            Serial.write(ResponseBuffer.data[i]);

        const uint8_t crc = Calc_CRC(
            ResponseBuffer.id,
            ResponseBuffer.data,
            ResponseBuffer.length
        );

        Serial.write(crc);
    }

    ResponseBuffer.responsePending = false;
}

void UART::sendNack(UART_CommandStatus_t status)
{
    const uint8_t reason = static_cast<uint8_t>(status);
    setResponse(STATUS_NACK, &reason, 1);
    sendResponse();
}

void UART::sendHello()
{
    Serial.println(F("Hello my friend"));
}

void UART::setError(uint8_t errorCode)
{
    if ((UART_context != nullptr) && (UART_context->error != nullptr))
        UART_context->error->setError(errorCode);
}

void UART::clearError()
{
    if ((UART_context != nullptr) && (UART_context->error != nullptr))
        UART_context->error->clearError(FY_ERROR_LOCATION_UART);
}

void UART::setResponse(uint8_t id, const uint8_t* data, uint8_t length)
{
    if (length > MAX_RESPONSE_LENGTH)
        length = MAX_RESPONSE_LENGTH;

    ResponseBuffer.id = id;
    ResponseBuffer.length = length;
    ResponseBuffer.container = ResponseContainer::Data;
    ResponseBuffer.string = nullptr;

    for (uint8_t i = 0; i < length; ++i)
        ResponseBuffer.data[i] = data[i];

    ResponseBuffer.responsePending = true;
}

void UART::setResponse(uint8_t id, PGM_P data)
{
    ResponseBuffer.id = id;
    ResponseBuffer.length = 0;
    ResponseBuffer.container = ResponseContainer::String;
    ResponseBuffer.string = data;
    ResponseBuffer.responsePending = true;
}

bool UART::CheckBusy()
{
    return true;
}

bool UART::CheckError()
{
    return true;
}

bool UART::SetCommand()
{
    MotorJob_t job{};

    if (FY_ModuleContext == nullptr || FY_ModuleContext->motor == nullptr)
        return false;

    job.cmd = CommandBuffer.command.cmd;

    for (uint8_t i = 0; i < MAX_COMMAND_LENGTH - 1; ++i)
        job.data[i] = CommandBuffer.command.data[i];

    return FY_ModuleContext->motor->setJob(job);
}

uint8_t UART::Calc_CRC(uint8_t id, const uint8_t* data, uint8_t length)
{
    uint8_t crc = id;

    for (uint8_t i = 0; i < length; ++i)
    {
        crc ^= data[i];

        for (uint8_t bit = 0; bit < 8; ++bit)
        {
            if (crc & 0x80)
                crc = static_cast<uint8_t>((crc << 1) ^ 0x07);
            else
                crc <<= 1;
        }
    }

    return crc;
}

uint8_t UART::Calc_CRC_PGM(uint8_t id, PGM_P data, uint8_t length)
{
    uint8_t crc = id;

    for (uint8_t i = 0; i < length; ++i)
    {
        crc ^= pgm_read_byte(&data[i]);

        for (uint8_t bit = 0; bit < 8; ++bit)
        {
            if (crc & 0x80)
                crc = static_cast<uint8_t>((crc << 1) ^ 0x07);
            else
                crc <<= 1;
        }
    }

    return crc;
}

UART_CommandStatus_t UART::getCommandStatus()
{
    return CommandBuffer.status;
}

void UART::clearCommandStatus()
{
    CommandBuffer.status = UART_CommandStatus_t::VALID;
}

void UART::handleStop()
{
    setResponse(
        STATUS_System,
        reinterpret_cast<const uint8_t*>(UART_context->systemStatus),
        sizeof(FY_SystemStatus_t)
    );
}

void UART::handleGetStatus()
{
    const uint8_t selector = CommandBuffer.command.data[0];
    if ((UART_context == nullptr) || (UART_context->systemStatus == nullptr))
    {
        const uint8_t reason = static_cast<uint8_t>(UART_CommandStatus_t::DATA_INVALID);
        setResponse(STATUS_NACK, &reason, 1U);
        return;
    }
    switch (selector)
    {
    case STATUS_SELECTOR_SYSTEM:
    {
        const FY_SystemInitStatus_t& init = UART_context->systemStatus->init;
        uint8_t data[2] = {0U, 0U};
        data[0] = static_cast<uint8_t>(
            (init.displayConnected ? 0x01U : 0U) |
            (init.portExpanderConnected ? 0x02U : 0U) |
            (init.uartConnected ? 0x04U : 0U) |
            (init.motorInitialized ? 0x08U : 0U) |
            (init.buttonsInitialized ? 0x10U : 0U) |
            (init.switchesInitialized ? 0x20U : 0U) |
            (init.systemError ? 0x40U : 0U));
        data[1] = static_cast<uint8_t>(UART_context->systemStatus->state);
        setResponse(STATUS_System, data, 2U);
        break;
    }
    case STATUS_SELECTOR_MOTOR:
    {
        uint8_t data[3] = {0U, 0U, 0U};
        if ((FY_ModuleContext != nullptr) && (FY_ModuleContext->motor != nullptr))
        {
            data[0] = static_cast<uint8_t>(
                (FY_ModuleContext->motor->isDriverActive() ? 0x01U : 0U) |
                (FY_ModuleContext->motor->isMoving() ? 0x02U : 0U));
        }
        setResponse(STATUS_Motor, data, 3U);
        break;
    }
    case STATUS_SELECTOR_SWITCHES:
    {
        uint8_t data = 0U;
        if ((FY_ModuleContext != nullptr) && (FY_ModuleContext->switches != nullptr))
        {
            data = static_cast<uint8_t>(
                (FY_ModuleContext->switches->getDigitalValue(Switches::Id::REF) ? 0x01U : 0U) |
                (FY_ModuleContext->switches->getDigitalValue(Switches::Id::TRIM_LEFT) ? 0x02U : 0U) |
                (FY_ModuleContext->switches->getDigitalValue(Switches::Id::TRIM_RIGHT) ? 0x04U : 0U) |
                (FY_ModuleContext->switches->getDigitalValue(Switches::Id::TIMING_BELT) ? 0x08U : 0U));
        }
        setResponse(STATUS_Switches, &data, 1U);
        break;
    }
    case STATUS_SELECTOR_KEYBOARD:
    {
        const uint8_t data = static_cast<uint8_t>(
            ((FY_ModuleContext != nullptr) &&
             (FY_ModuleContext->keyboard != nullptr) &&
             FY_ModuleContext->keyboard->is_connected()) ? 0x01U : 0U);
        setResponse(STATUS_Keyboard, &data, 1U);
        break;
    }
    default:
    {
        const uint8_t reason = static_cast<uint8_t>(UART_CommandStatus_t::DATA_INVALID);
        setError(UART_ERROR_INVALID_DATA);
        setResponse(STATUS_NACK, &reason, 1U);
        break;
    }
    }
}

void UART::handleGetError()
{
    uint8_t data[4] = {0U, 0U, 0U, 0U};

    if ((UART_context != nullptr) && (UART_context->error != nullptr))
    {
        data[0] = UART_context->error->getSystemByte();
        data[1] = UART_context->error->getError(0U);
        data[2] = UART_context->error->getError(1U);
        data[3] = UART_context->error->getError(2U);
    }

    setResponse(STATUS_Error, data, 4U);
}

void UART::handleGetPosition()
{
    setResponse(
        STATUS_Position,
        reinterpret_cast<const uint8_t*>(UART_context->motorPosition),
        sizeof(int32_t)
    );
}

void UART::handleGetTrack()
{
    setResponse(
        STATUS_Track,
        reinterpret_cast<const uint8_t*>(UART_context->Track_INFO),
        1
    );
}

void UART::handleHelp()
{
    static const char HELP_RESPONSE[] PROGMEM =
        "GET_STATUS GET_ERROR GET_POSITION GET_TRACK HELP GET_FIRMWARE";

    setResponse(STATUS_Help, HELP_RESPONSE);
}

void UART::handleGetFirmware()
{
    setResponse(STATUS_Help, FW_RESPONSE);
}

void UART::handle_Busy()
{
    // Execute commands rejected while the controller is busy receive NACK.
    const uint8_t commandId = CommandBuffer.command.cmd;
    setResponse(STATUS_NACK, &commandId, 1U);
}

void UART::handle_ACK()
{
    const uint8_t commandId = CommandBuffer.command.cmd;
    setResponse(STATUS_ACK, &commandId, 1U);
}

void UART::handleReference()
{
    SetCommand();
}

void UART::handleSetSpeed()
{
    SetCommand();
}

void UART::handleGo()
{
    SetCommand();
}

void UART::handleLeft()
{
    SetCommand();
}

void UART::handleRight()
{
    SetCommand();
}

void UART::handleSetPosition()
{
    SetCommand();
}

void UART::handleSetTrack()
{
    if ((CommandBuffer.command.data[0] >= static_cast<uint8_t>(FY_Track::BG1)) &&
        (CommandBuffer.command.data[0] <= static_cast<uint8_t>(FY_Track::BG5)))
    {
        // Track validation will be completed with the motor command path.
    }
}

void UART::handleSetMode()
{
    // Issue #58: DATA = 55 AA for LOCAL, AA 55 for REMOTE.
    const uint8_t first = CommandBuffer.command.data[0];
    const uint8_t second = CommandBuffer.command.data[1];
    const uint8_t commandId = CommandBuffer.command.cmd;

    if (UART_context == nullptr || UART_context->systemStatus == nullptr ||
        FY_ModuleContext == nullptr || FY_ModuleContext->keyboard == nullptr ||
        FY_ModuleContext->motor == nullptr)
    {
        setResponse(STATUS_NACK, &commandId, 1U);
        return;
    }

    // Do not change operating mode while the controller is busy or the motor moves.
    if (UART_context->systemStatus->state == FY_SystemState_t::Busy ||
        FY_ModuleContext->motor->isMoving())
    {
        setResponse(STATUS_NACK, &commandId, 1U);
        return;
    }

    if (first == 0x55U && second == 0xAAU)
    {
        FY_ModuleContext->keyboard->setMode(Keyboard::Mode::LOCAL);
        setResponse(STATUS_ACK, &commandId, 1U);
        return;
    }

    if (first == 0xAAU && second == 0x55U)
    {
        FY_ModuleContext->keyboard->setMode(Keyboard::Mode::REMOTE);
        setResponse(STATUS_ACK, &commandId, 1U);
        return;
    }

    setResponse(STATUS_NACK, &commandId, 1U);
}
