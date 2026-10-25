#include "BabelFish.h"
#include "Motor.h"

namespace
{
    const char* nextToken(char*& text)
    {
        while (*text == ' ' || *text == '\t')
            ++text;

        if (*text == '\0')
            return nullptr;

        char* start = text;

        while (*text != '\0' && *text != ' ' && *text != '\t')
            ++text;

        if (*text != '\0')
        {
            *text = '\0';
            ++text;
        }

        return start;
    }
}

void BabelFish::begin(uint32_t baudRate)
{
    Serial.begin(baudRate);
    _inputLength = 0;
    _input[0] = '\0';
    _command = {};
}

void BabelFish::update()
{
    while (Serial.available())
    {
        BabelFishCommand_t command{};
        process(
            static_cast<char>(Serial.read()),
            command
        );
    }
}

BabelFishResult BabelFish::process(
    char c,
    BabelFishCommand_t& command)
{
    command = {};

    // Local echo: send every typed character back immediately.
    // CR/LF are handled as line termination below.
    if (c == '\r')
    {
        Serial.println();
        _input[_inputLength] = '\0';
        processLine();
        _inputLength = 0;
        _input[0] = '\0';

        if (_command.valid)
        {
            command = _command;
            _command.valid = false;
            return BabelFishResult::COMMAND_READY;
        }

        return BabelFishResult::INVALID;
    }

    if (c == '\n')
    {
        // Accept terminals configured for LF or CR/LF.
        // After CR the line has already been processed.
        if (_inputLength == 0)
            return BabelFishResult::NONE;

        Serial.println();
        _input[_inputLength] = '\0';
        processLine();
        _inputLength = 0;
        _input[0] = '\0';

        if (_command.valid)
        {
            command = _command;
            _command.valid = false;
            return BabelFishResult::COMMAND_READY;
        }

        return BabelFishResult::INVALID;
    }

    // Basic terminal backspace handling.
    if (c == '\b' || c == 0x7F)
    {
        if (_inputLength > 0)
        {
            --_inputLength;
            Serial.print(F("\b \b"));
        }

        return BabelFishResult::NONE;
    }

    // Echo ordinary characters immediately.
    Serial.write(static_cast<uint8_t>(c));

    if (_inputLength < INPUT_LENGTH)
    {
        _input[_inputLength++] = c;
    }
    else
    {
        // Keep the line bounded. The complete line will be rejected.
        _input[_inputLength] = '\0';
    }

    return BabelFishResult::NONE;
}

bool BabelFish::commandAvailable() const
{
    return _command.valid;
}

bool BabelFish::getCommand(BabelFishCommand_t& command)
{
    if (!_command.valid)
        return false;

    command = _command;
    _command.valid = false;
    return true;
}

void BabelFish::processLine()
{
    if (_inputLength == 0)
        return;

    trim(_input);

    if (_input[0] == '\0')
        return;

    if (!decodeLine())
    {
        reportInvalid();
        return;
    }

    reportBinaryCommand();
}

bool BabelFish::decodeLine()
{
    char* cursor = _input;
    const char* commandName = nextToken(cursor);

    if (commandName == nullptr)
        return false;

    uint8_t cmd = 0;

    // BabelFish resolves only the human-readable command name.
    // DATA is supplied separately as hexadecimal byte tokens.
    if (equalsIgnoreCase(commandName, "GET_STATUS") ||
        equalsIgnoreCase(commandName, "STATUS"))
        cmd = CMD_GET_STATUS;
    else if (equalsIgnoreCase(commandName, "GET_ERROR") ||
             equalsIgnoreCase(commandName, "ERROR"))
        cmd = CMD_GET_ERROR;
    else if (equalsIgnoreCase(commandName, "GET_POSITION") ||
             equalsIgnoreCase(commandName, "POSITION"))
        cmd = CMD_GET_POSITION;
    else if (equalsIgnoreCase(commandName, "GET_TRACK") ||
             equalsIgnoreCase(commandName, "TRACK"))
        cmd = CMD_GET_TRACK;
    else if (equalsIgnoreCase(commandName, "HELP"))
        cmd = CMD_HELP;
    else if (equalsIgnoreCase(commandName, "GET_FIRMWARE") ||
             equalsIgnoreCase(commandName, "FIRMWARE"))
        cmd = CMD_GET_FIRMWARE;
    else if (equalsIgnoreCase(commandName, "REFERENCE"))
        cmd = CMD_REFERENCE;
    else if (equalsIgnoreCase(commandName, "SET_PARAM"))
        cmd = CMD_SET_PARAM;
    else if (equalsIgnoreCase(commandName, "GO"))
        cmd = CMD_GO;
    else if (equalsIgnoreCase(commandName, "MOVE"))
        cmd = fy::CMD_MOVE;
    else if (equalsIgnoreCase(commandName, "LEFT"))
        cmd = CMD_LEFT;  // Legacy Nano command; rejected by ESP32 motor router.
    else if (equalsIgnoreCase(commandName, "RIGHT"))
        cmd = CMD_RIGHT; // Legacy Nano command; rejected by ESP32 motor router.
    else if (equalsIgnoreCase(commandName, "SET_POSITION"))
        cmd = CMD_SET_POSITION;
    else if (equalsIgnoreCase(commandName, "SET_TRACK"))
        cmd = CMD_SET_TRACK;
    else if (equalsIgnoreCase(commandName, "SET_MODE"))
        cmd = CMD_SET_MODE;
    else if (equalsIgnoreCase(commandName, "STOP") ||
             equalsIgnoreCase(commandName, "STOPP"))
        cmd = CMD_STOPP;
    else
        return false;

    _command = {};
    _command.cmd = cmd;

    uint8_t dataLength = 0;
    const char* token = nullptr;

    while ((token = nextToken(cursor)) != nullptr)
    {
        if (dataLength >= (MAX_COMMAND_LENGTH - 1U))
        {
            _command = {};
            return false;
        }

        uint8_t value = 0;
        if (!parseHexByte(token, value))
        {
            _command = {};
            return false;
        }

        _command.data[dataLength] = value;
        ++dataLength;
    }

    return finalizeCommand(cmd, dataLength);
}

bool BabelFish::finalizeCommand(uint8_t cmd, uint8_t dataLength)
{
    // Do not validate the command-specific DATA length here.
    // UART::validateCommand() owns that protocol-level validation.
    if (dataLength > (MAX_COMMAND_LENGTH - 1U))
    {
        _command = {};
        return false;
    }

    _command.cmd = cmd;
    _command.length = static_cast<uint8_t>(dataLength + 1U);
    _command.valid = true;
    _command.crc = calcCRC(cmd, _command.data, dataLength);
    return true;
}

uint8_t BabelFish::calcCRC(
    uint8_t cmd,
    const uint8_t* data,
    uint8_t length)
{
    uint8_t crc = cmd;

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

void BabelFish::reportInvalid() const
{
    Serial.println(F("CMD_INVALID"));
}

void BabelFish::reportBinaryCommand() const
{
    Serial.print(F("CMD=0x"));

    if (_command.cmd < 0x10)
        Serial.print('0');

    Serial.print(_command.cmd, HEX);
    Serial.print(F(" LEN="));
    Serial.print(_command.length);
    Serial.print(F(" CRC=0x"));

    if (_command.crc < 0x10)
        Serial.print('0');

    Serial.println(_command.crc, HEX);
}

void BabelFish::trim(char* text)
{
    char* start = text;

    while (*start == ' ' || *start == '\t')
        ++start;

    if (start != text)
    {
        char* dst = text;

        while (*start != '\0')
            *dst++ = *start++;

        *dst = '\0';
    }

    uint8_t length = 0;

    while (text[length] != '\0')
        ++length;

    while (length > 0 &&
           (text[length - 1] == ' ' || text[length - 1] == '\t'))
    {
        text[--length] = '\0';
    }
}

bool BabelFish::equalsIgnoreCase(const char* left, const char* right)
{
    while (*left != '\0' && *right != '\0')
    {
        char a = *left;
        char b = *right;

        if (a >= 'a' && a <= 'z')
            a = static_cast<char>(a - 'a' + 'A');

        if (b >= 'a' && b <= 'z')
            b = static_cast<char>(b - 'a' + 'A');

        if (a != b)
            return false;

        ++left;
        ++right;
    }

    return (*left == '\0') && (*right == '\0');
}

bool BabelFish::parseHexByte(const char* text, uint8_t& value)
{
    if ((text == nullptr) || (*text == '\0'))
        return false;
    uint16_t parsed = 0;
    uint8_t digits = 0;
    while (*text != '\0')
    {
        uint8_t nibble = 0;
        if (*text >= '0' && *text <= '9')
            nibble = static_cast<uint8_t>(*text - '0');
        else if (*text >= 'A' && *text <= 'F')
            nibble = static_cast<uint8_t>(*text - 'A' + 10);
        else if (*text >= 'a' && *text <= 'f')
            nibble = static_cast<uint8_t>(*text - 'a' + 10);
        else
            return false;
        parsed = static_cast<uint16_t>((parsed << 4) | nibble);
        if (++digits > 2)
            return false;
        ++text;
    }
    value = static_cast<uint8_t>(parsed);
    return digits > 0;
}

