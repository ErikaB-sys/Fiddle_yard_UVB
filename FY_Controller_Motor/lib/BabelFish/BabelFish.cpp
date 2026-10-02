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
        const char c = static_cast<char>(Serial.read());

        if (c == '\r')
            continue;

        if (c == '\n')
        {
            _input[_inputLength] = '\0';
            processLine();
            _inputLength = 0;
            _input[0] = '\0';
            continue;
        }

        if (_inputLength < INPUT_LENGTH)
        {
            _input[_inputLength++] = c;
        }
        else
        {
            // Keep the line bounded. The complete line will be rejected.
            _input[_inputLength] = '\0';
        }
    }
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
    echoLine();

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
        return makeSimpleCommand(CMD_GET_STATUS);

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
        return true;
    }

    if (equalsIgnoreCase(command, "SET_SPEED"))
    {
        const char* value = nextToken(cursor);
        uint16_t speed = 0;

        if ((value == nullptr) || !parseUInt16(value, speed))
            return false;

        _command.cmd = CMD_SET_SPEED;
        _command.data[0] = static_cast<uint8_t>(speed & 0xFF);
        _command.data[1] = static_cast<uint8_t>(speed >> 8);
        _command.length = 3;
        _command.valid = true;
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
    return true;
}

void BabelFish::echoLine() const
{
    Serial.print(F("ECHO: "));
    Serial.println(_input);
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
    Serial.println(_command.length);
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
