#pragma once

#include <Arduino.h>
#include "Protokoll.h"

/**
 * @brief ASCII-to-command adapter for the controller UART.
 *
 * BabelFish accepts human-readable commands on the serial interface,
 * echoes the complete input line, and converts recognized commands
 * into the binary command representation used by the controller.
 *
 * The adapter does not execute commands. It only translates them.
 */
struct BabelFishCommand_t
{
    uint8_t cmd{0};
    uint8_t data[MAX_COMMAND_LENGTH - 1]{};
    uint8_t length{0};
    bool valid{false};
};

class BabelFish
{
public:
    void begin(uint32_t baudRate = 115200);
    void update();

    bool commandAvailable() const;
    bool getCommand(BabelFishCommand_t& command);

private:
    static constexpr uint8_t INPUT_LENGTH = 63;

    char _input[INPUT_LENGTH + 1]{};
    uint8_t _inputLength = 0;

    BabelFishCommand_t _command{};

    void processLine();
    bool decodeLine();

    static void trim(char* text);
    static bool equalsIgnoreCase(const char* left, const char* right);
    static bool parseUInt8(const char* text, uint8_t& value);
    static bool parseUInt16(const char* text, uint16_t& value);

    bool makeSimpleCommand(uint8_t cmd);
    void echoLine() const;
    void reportInvalid() const;
    void reportBinaryCommand() const;
};
