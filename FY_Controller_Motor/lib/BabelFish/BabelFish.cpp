#include "BabelFish.h"

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
    const char* command = nextToken(cursor);

    if (command == nullptr)
        return false;

    _command = {};

    // Immediate commands.
    if (equalsIgnoreCase(command, "GET_STATUS") ||
        equalsIgnoreCase(command, "STATUS"))
    {
        const char* value = nextToken(cursor);
        uint8_t selector = 0;
        if ((value == nullptr) || !parseHexByte(value, selector))
            return false;
        _command.cmd = CMD_GET_STATUS;
        _command.data[0] = selector;
        _command.length = 2;
        _command.valid = true;
        _command.crc = calcCRC(_command.cmd, _command.data, 1);
        return true;
    }

    if (equalsIgnoreCase(command, "GET_ERROR") ||
        equalsIgnoreCase(command, "ERROR"))
        return makeSimpleCommand(CMD_GET_ERROR);

    if (equalsIgnoreCase(command, "GET_POSITION") ||
        equalsIgnoreCase(command, "POSITION"))
        return makeSimpleCommand(CMD_GET_POSITION);

    if (equalsIgnoreCase(command, "GET_TRACK") ||
        equalsIgnoreCase(command, "TRACK"))
        return makeSimpleCommand(CMD_GET_TRACK);

    if (equalsIgnoreCase(command, "HELP"))
        return makeSimpleCommand(CMD_HELP);

    if (equalsIgnoreCase(command, "GET_FIRMWARE") ||
        equalsIgnoreCase(command, "FIRMWARE"))
        return makeSimpleCommand(CMD_GET_FIRMWARE);

    // Operating mode: SET_MODE LOCAL | SET_MODE REMOTE (Issue #58).
    if (equalsIgnoreCase(command, "SET_MODE"))
    {
        const char* value = nextToken(cursor);
        if (value == nullptr)
            return false;

        uint8_t first = 0;
        uint8_t second = 0;
        if (equalsIgnoreCase(value, "LOCAL"))
        {
            first = 0x55;
            second = 0xAA;
        }
        else if (equalsIgnoreCase(value, "REMOTE"))
        {
            first = 0xAA;
            second = 0x55;
        }
        else
        {
            return false;
        }

        _command.cmd = CMD_SET_MODE;
        _command.data[0] = first;
        _command.data[1] = second;
        _command.length = 3;
        _command.valid = true;
        _command.crc = calcCRC(_command.cmd, _command.data, 2);
        return true;
    }

    // Execute / priority commands.
    if (equalsIgnoreCase(command, "REFERENCE"))
        return makeSimpleCommand(CMD_REFERENCE);

    if (equalsIgnoreCase(command, "STOP"))
        return makeSimpleCommand(CMD_STOPP);

    if (equalsIgnoreCase(command, "LEFT"))
        return makeSimpleCommand(CMD_LEFT);

    if (equalsIgnoreCase(command, "RIGHT"))
        return makeSimpleCommand(CMD_RIGHT);

    if (equalsIgnoreCase(command, "SET_POSITION"))
    {
        const char* value = nextToken(cursor);
        uint16_t position = 0;

        if ((value == nullptr) || !parseUInt16(value, position))
            return false;

        _command.cmd = CMD_SET_POSITION;
        _command.data[0] = static_cast<uint8_t>(position & 0xFF);
        _command.data[1] = static_cast<uint8_t>(position >> 8);
        _command.length = 3;
        _command.valid = true;
        _command.crc = calcCRC(
            _command.cmd,
            _command.data,
            _command.length - 1
        );
        return true;
    }

    if (equalsIgnoreCase(command, "SET_TRACK"))
    {
        const char* value = nextToken(cursor);
        uint8_t track = 0;

        if ((value == nullptr) || !parseUInt8(value, track))
            return false;

        if ((track < 1) || (track > 5))
            return false;

        _command.cmd = CMD_SET_TRACK;
        _command.data[0] = track;
        _command.length = 2;
        _command.valid = true;
        _command.crc = calcCRC(
            _command.cmd,
            _command.data,
            _command.length - 1
        );
        return true;
    }

    // Generic parameter command: SET_PARAM <parameter-id> <value>
    // DATA[0] = parameter ID; DATA[1..3] = 24-bit value, little-endian.
    // Parameter semantics and ranges are specified in Issues #145 and #58.
    if (equalsIgnoreCase(command, "SET_PARAM"))
    {
        const char* idToken = nextToken(cursor);
        const char* valueToken = nextToken(cursor);
        uint8_t parameterId = 0;
        uint32_t parameterValue = 0;

        if ((idToken == nullptr) || !parseUInt8(idToken, parameterId) ||
            (valueToken == nullptr) || !parseUInt24(valueToken, parameterValue))
            return false;

        _command.cmd = CMD_SET_PARAM;
        _command.data[0] = parameterId;
        _command.data[1] = static_cast<uint8_t>(parameterValue & 0xFFUL);
        _command.data[2] = static_cast<uint8_t>((parameterValue >> 8) & 0xFFUL);
        _command.data[3] = static_cast<uint8_t>((parameterValue >> 16) & 0xFFUL);
        _command.length = 5;
        _command.valid = true;
        _command.crc = calcCRC(
            _command.cmd,
            _command.data,
            _command.length - 1
        );
        return true;
    }

    // CMD_GO is deliberately not exposed by the ASCII adapter.
    return false;
}

bool BabelFish::makeSimpleCommand(uint8_t cmd)
{
    _command.cmd = cmd;
    _command.length = 1;
    _command.valid = true;
    _command.crc = calcCRC(_command.cmd, nullptr, 0);
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

bool BabelFish::parseUInt8(const char* text, uint8_t& value)
{
    uint16_t parsed = 0;

    if (!parseUInt16(text, parsed) || parsed > 255)
        return false;

    value = static_cast<uint8_t>(parsed);
    return true;
}

bool BabelFish::parseUInt16(const char* text, uint16_t& value)
{
    if ((text == nullptr) || (*text == '\0'))
        return false;

    uint32_t parsed = 0;

    while (*text != '\0')
    {
        if (*text < '0' || *text > '9')
            return false;

        parsed = (parsed * 10UL) + static_cast<uint8_t>(*text - '0');

        if (parsed > 65535UL)
            return false;

        ++text;
    }

    value = static_cast<uint16_t>(parsed);
    return true;
}

bool BabelFish::parseUInt24(const char* text, uint32_t& value)
{
    if ((text == nullptr) || (*text == '\0'))
        return false;

    uint32_t parsed = 0;

    while (*text != '\0')
    {
        if (*text < '0' || *text > '9')
            return false;

        parsed = (parsed * 10UL) + static_cast<uint8_t>(*text - '0');

        if (parsed > 0xFFFFFFUL)
            return false;

        ++text;
    }

    value = parsed;
    return true;
}
