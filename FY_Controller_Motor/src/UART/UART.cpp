#include <Arduino.h>
#include "Protokoll.h"
#include "UART.h"

   UART:: UART()
   {// define  some UART data 
    uint8_t UART_Error = 0;


    //Init some Data 
     UART_context= nullptr;



   }
    

    void begin(UART_Context_t& context)
    { // Check if  content is set
          if ( UART_context ==  nullptr)
        {
        UART_context = &context;
         }
         else
         {
         UART_Error |= UART_ERROR_NO_CONTENT;
         }
     // try to connect to the master e.g.ESP32
         Serial.begin( UART_BAUD_RATE);
        if (Serial.available())
        {UART::sendHello();}
     // what will be the Answer ? 
        


    }

    /* cyclic funktion to receive and send date from / to the master on serieal */
    void update()
    {


    }



    void UART::receive()
    {
      
    while (Serial.available())
    {
        uint8_t byte = static_cast<uint8_t>(Serial.read());

        // Byte in commandBuffer schreiben
        // Länge prüfen
        // Telegramm vollständig?
    } 



    }

    void decodeCommand()
    {   

    }

    void sendResponse()
    {

    }

    void sendHello()
    {
     Serial.println (F("Hello my friend"));
     
    }
