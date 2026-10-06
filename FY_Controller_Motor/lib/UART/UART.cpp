#include <Arduino.h>
#include <HardwareSerial.h>
#include "FY_System.h"
#include "Protokoll.h"
#include "UART.h"
#include "Error.h"
#include "Firmware.h"
#include "Motor.h"

UART::UART()
{
    UART_context = nullptr;
    FY_ModuleContext = nullptr;
    UART_Error = 0;

    CommandBuffer = {};
    CommandBuffer.status = UART_CommandStatus_t::VALID;

    ResponseBuffer = {};
}

void UART::begin(UART_Context_t& context, FY_ModuleContext_t& modules)
{
    Serial.begin(UART_BAUD_RATE);

    UART_context = &context;
    FY_ModuleContext = &modules;

    _babelFish.begin();

    sendHello();
}

void UART::update()
{
    if (UART_context == nullptr)
    {
        UART_Error |= UART_ERROR_NO_CONTENT;
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

        if (commandDefinitions[i].telegramLength != CommandBuffer.command.length)
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
        CommandBuffer.response = commandDefinitions[i].response;
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

    case CMD_SET_REMOTE:
        handleSetRemote();
        break;

    case CMD_SET_LOCAL:
        handleSetLocal();
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

    if (ResponseBuffer.length == RESPONSE_LENGTH_STRING)
    {
        const uint8_t length = static_cast<uint8_t>(strlen_P(ResponseBuffer.string));

        for (uint8_t i = 0; i < length; ++i)
            Serial.write(pgm_read_byte(&ResponseBuffer.string[i]));

        const uint8_t crc = Calc_CRC(
            ResponseBuffer.id,
            reinterpret_cast<const uint8_t*>(ResponseBuffer.string),
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

void UART::setResponse(uint8_t id, const uint8_t* data, uint8_t length)
{
    if (length > MAX_RESPONSE_LENGTH)
        length = MAX_RESPONSE_LENGTH;

    ResponseBuffer.id = id;
    ResponseBuffer.length = length;
    ResponseBuffer.string = nullptr;

    for (uint8_t i = 0; i < length; ++i)
        ResponseBuffer.data[i] = data[i];

    ResponseBuffer.responsePending = true;
}

void UART::setResponse(uint8_t id, PGM_P data)
{
    ResponseBuffer.id = id;
    ResponseBuffer.length = RESPONSE_LENGTH_STRING;
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
    setResponse(
        STATUS_System,
        reinterpret_cast<const uint8_t*>(UART_context->systemStatus),
        sizeof(FY_SystemStatus_t)
    );
}

void UART::handleGetError()
{
    setResponse(
        STATUS_Error,
        reinterpret_cast<const uint8_t*>(&UART_context->systemStatus->error),
        sizeof(FY_System_Error_t)
    );
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
    // Busy response still to be defined.
}

void UART::handle_ACK()
{
    // ACK response still to be defined.
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

void UART::handleSetRemote()
{
}

void UART::handleSetLocal()
{
}
