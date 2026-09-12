#pragma once
#include <Arduino.h>
#include <stdio.h>


class UART {
public:
    UART(const char* device, int baud);
    ~UART();

    bool begin(unsigned long B_rate); 
    uint8_t update(uint8_t *buffer,size_t length  );
 // special function to send help message over Uart
    bool send_help();
    bool send_Error(uint8_t* error_code);

private:
    int baud_rate{0};
    int uart_fd{-1};
    const char* uart_device{nullptr};
    bool sendData(const uint8_t* data, size_t length);
    bool receiveData(uint8_t* buffer, size_t length);
  
};

