#include <Arduino.h>
#include "Protokoll.h"
#include "UART.h"
class UART
{


public:

     UART(const char* device, int baud){
        // Constructor to initialize UART with the specified UART device  and baud rate
        // This function should store the baud rate for later use
        };


    ~UART(){
        // Destructor to clean up resources
        // This function should close the UART device if it is open
        };

    bool begin(unsigned long B_rate){
        // Initialize UART communication with the specified device and baud rate
        // This function should open the UART device and configure it for communication
        // Return true if successful, false otherwise
       Serial.begin(B_rate);
       // Check Serial if data avalible 
       Serial.println (F("Hello my Frend"));
       Serial.flush();
       return ( true);

    
    };

    uint8_t update(uint8_t *buffer,size_t length  ){
        // Update the UART communication by sending and receiving data
        // This function should handle the sending of commands and receiving of responses
        // Return true if successful, false otherwise


    };


  
    int baud_rate{0};
    int uart_fd{-1};
    const char* uart_device{nullptr};
    bool send_help()
    {
        // This function should send a help message over UART
        // The help message should contain information about the available commands and their usage
        // Return true if successful, false otherwise


    };
    bool send_Error(uint8_t* error_code){
        // This function should send an error message over UART
        // The error message should contain the specified error code
        // Return true if successful, false otherwise


    };
    


    bool sendData(const uint8_t* data, size_t length){
        // Send data over UART
        // This function should write the specified data to the UART device
        // Return true if successful, false otherwise
    };
    bool receiveData(uint8_t* buffer, size_t length){
        // Receive data over UART
        // This function should read data from the UART device into the specified buffer
        // Return true if successful, false otherwise
    };
}