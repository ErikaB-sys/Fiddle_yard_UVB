#pragma once

#include <Arduino.h>

struct UART_Context
{
    uint8_t*  status;
    uint8_t*  error;
    int32_t*  position;
    uint8_t*  track;
    uint16_t* speed;

};

class UART
{
public:
    UART();

    void begin(UART_Context& context);
    void update();

private:

    UART_Context* context;

    void receive();
    void decodeCommand();
    void sendResponse();
    void sendHello();
};