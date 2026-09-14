
#include <Arduino.h>
#include <HardwareSerial.h>
#include "FY_System.h"
#include "Config.h"
#include "Protokoll.h"
#include "UART.h"
/// @todo  bool _ResponsePending; um die korekte antwort zu senden 
///UART empfängt CMD
///        ↓
///Command an Motor
///        ↓
///Motor.Update()
///        ↓
///Ergebnis / Fehler
///        ↓
///ResponsePending = true
///        ↓
///UART.update()
///        ↓
///Response senden
///        ↓
///ResponsePending = false


   UART:: UART()
   {
       //Init some Data 
       UART_context= nullptr;
       UART_Error = 0;
       uint8_t data[MAX_COMMAND_LENGTH - 1]{};

       commandLength =1;
       CommandBuffer.CMD_valid= false;



   }
    
  
    ///@brief 
    /// @param context 
    void UART:: begin(UART_Context_t& context)
    { // Check if  content is set
         Serial.begin( UART_BAUD_RATE);
         this-> UART_context = &context;
      // try to connect to the master e.g.ESP32
            sendHello();
            
             // what will be the Answer ? 
    }
    
    

    /* cyclic funktion to receive and send date from / to the master on serieal */
    void UART::update()
    {
     if ( this -> UART_context ==  nullptr)
             {       
            UART_Error |= UART_ERROR_NO_CONTENT;
            Serial.println(F("No content. The UART is feeling lonely."));
            return;
             }
        receive();
        decodeCommand();
        sendResponse();
             



    }



    /**
     * @brief Receives and processes one UART telegram byte at a time.
     *
     * The receive state machine searches for a valid command, reads the
     * command's expected payload, and then invokes CRC checking.
     */
    void UART::receive()
    {
    switch (receiveState)
        {
         case ReceiveState::FindCommand:
          // Read and validate a command byte when data is available.
          if (Serial.available())
          {
          uint8_t cmd = Serial.read();
          // Compare the received byte with all known command definitions.
          for (uint8_t i = 0; i < COMMAND_COUNT; i++)
           {
               if (commandDefinitions[i].id == cmd)
               {   CommandBuffer.cmd = cmd;
                   CommandBuffer.type = commandDefinitions[i].type;
                   CommandBuffer.response = commandDefinitions[i].response; 

                   // Store the expected payload length and prepare for data reception.
                   expectedLength = commandDefinitions[i].telegramLength-1; // CMd  is member  of  the lenght !
                   receiveState = ReceiveState::ReadData;
                   dataIndex = 0;
                   break;
               }
            }
           // No valid command was found; remain in the command-search state.
           }
             break;
            case ReceiveState::ReadData:

                // Read payload bytes while data is available.
                if (Serial.available())
                {
                    CommandBuffer.data[dataIndex] = Serial.read();
                    dataIndex++;
                
                    // Move to CRC validation after the complete payload is received.
                    if (dataIndex >= expectedLength)
                    {
                        receiveState = ReceiveState::CheckCRC;
                    }
                }

             break;
     
         case ReceiveState::CheckCRC:
                    // Validate the received telegram checksum.
               Check_CRC();
                        // Return to command detection for the next telegram.
              receiveState = ReceiveState:: FindCommand ;
             break;

           default: 
           break;

        }

    } 
    
     /// @brief decode the Type 

     void UART::decodeCommand()
     {   
           switch (CommandBuffer.type)
         {
       case CommandType::IMMEDIATE :
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
        switch(CommandBuffer.cmd)
        {
        case   CMD_GET_STATUS   :
            handleGetStatus();
        break;

        case   CMD_GET_ERROR :
           handleGetError();
        break;
        case  CMD_GET_POSITION :
              handleGetPosition();
        break;
        case  CMD_GET_TRACK   :
                handleGetTrack();
        break;
        case CMD_HELP         :
                handleHelp();
        break;

        default     :
          CommandBuffer.CMD_valid = false;
        // Unknown Handle
        break;

        }



    }

    void UART::decodeExecute()
    {
        // Execute the command when the status permits command processing.
        // The current status is stored in the UART context.
        if (UART_context->systemStatus->state == FY_SystemState_t::Busy)
        {
            switch (CommandBuffer.cmd)
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
                // Mark unsupported commands as invalid.
                CommandBuffer.CMD_valid = false;
                break;
            }
        }
        else
        {// ignore Comand 
         CommandBuffer.CMD_valid = false;
        // and Send Busy message 

            }
    }
    void UART::decodePriority()
    {
       switch(CommandBuffer.cmd)
        {
        case  CMD_STOPP :
        handleStop();
        break;
        default:
        CommandBuffer.CMD_valid = false;
        // Unknown Handle
        break;
        }
    }

     void UART::sendResponse()
     {

    if (!ResponseBuffer.responsePending)
        return;

    Serial.write(ResponseBuffer.id);

    for (uint8_t i = 0; i < ResponseBuffer.length; ++i)
    {
        Serial.write(ResponseBuffer.data[i]);
    }

    uint8_t crc = Calc_CRC(
        ResponseBuffer.id,
        ResponseBuffer.data,
        ResponseBuffer.length
    );

    Serial.write(crc);

    ResponseBuffer.responsePending = false;
    }
    


     /// @brief 
     void UART::sendHello()
     {
     Serial.println (F("Hello my friend"));
     
     }
     void UART::setResponse(uint8_t id, uint8_t* data, uint8_t length)
        {
            ResponseBuffer.id = id;
            ResponseBuffer.data = data;
            ResponseBuffer.length = length;
            ResponseBuffer.responsePending = true;
        }
     bool UART::CheckBusy()
     {
        return (true);
     }

     bool UART::CheckError()
     {
        return (true);
     }

     bool UART::SetCommand()
     { 
      // übeträgt die daten in die MOVE Struktur 


       return( true);
     }
     




     ///@brief 
     bool UART:: Check_CRC()
     {
        /*
        https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/17
        */
         #ifdef UART_USE_CRC_RX
         // CRC prüfen
         #else
             CommandBuffer.CMD_valid = true;
         #endif
             return(CommandBuffer.CMD_valid);
         }
   
     

     uint8_t UART:: Calc_CRC (uint8_t id,const uint8_t* data,uint8_t length)
     {
          /*
         https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/18
         */
      #ifdef UART_USE_CRC_TX

       uint8_t crc = id;
      
          for (uint8_t i = 0; i < length; ++i)
          {
              crc ^= data[i];
      
              for (uint8_t bit = 0; bit < 8; ++bit)
              {
                  if (crc & 0x80)
                      crc = (crc << 1) ^ 0x07;
                  else
                      crc <<= 1;
              }
          }
      
          return crc;
      
      #else
      
          return 0xFF;
      
      #endif
     }






     ///@brief chapter handle fkt 

     /// @brief Stop fuktion Halt wthout any  discussions 
     void UART::handleStop()
     {
        // ENA auf OFF oder 12V  OFF setzen
        setResponse(STATUS_System,reinterpret_cast<uint8_t*>(UART_context->systemStatus),sizeof(FY_SystemStatus_t));

     }
    void  UART:: handleGetStatus()
    {
        setResponse(STATUS_System,reinterpret_cast<uint8_t*>(UART_context->systemStatus),sizeof(FY_SystemStatus_t));
    }
    void  UART:: handleGetError()
    {
         setResponse(STATUS_System,reinterpret_cast<uint8_t*>(UART_context->systemStatus->state),sizeof(FY_SystemState_t));
    }
 void UART::handleGetPosition()
    {
     setResponse(
         STATUS_Position, reinterpret_cast<uint8_t*>(UART_context->motorPosition),sizeof(int32_t));

    }
    void  UART:: handleGetTrack()
    {
        setResponse(STATUS_Track,reinterpret_cast<uint8_t*>(UART_context->motorTrack),1);
        
    }
    void  UART:: handleHelp()
    {
        // Special 
    }

   void UART:: handle_Busy()
   {
    //setResponse(STATUS_NACK, 0xFF ,1);
    

   }
     void UART:: handle_ACK()
   {
    //setResponse(STATUS_NACK, 0xFF ,1);
    

   }




    void  UART:: handleReference()
    {   // test Fehler 
        // Test  Busy 
        //testen ob  Referenziert 
        //SetCommand();
        
        
    }
    void  UART:: handleSetSpeed()
    {    // test Fehler 
        // Test  Busy
        //Test on Speed im bereich MIN/ MAX
        //SetCommand();

        
    }
    void  UART:: handleGo()
    {  // test Fehler 
        // Test  Busy
        //Test ob Steps >0 ( oder Steps min) 
        //SetCommand();
        
    }
    void  UART:: handleLeft()
    {  // test Fehler 
        // Test  Busy
        // Refernziert ?
        // Test ob noch Move Left möglich 
        //SetCommand();
        
    }
    void  UART:: handleRight()
    { // test Fehler 
        // Test  Busy
        // Refernziert ?
        // Test ob noch Move Right möglich 
        //SetCommand();
        
    }
    void  UART:: handleSetPosition()
    {   // test Fehler 
        // Test  Busy
        // Refernziert ?
        // Test ob POS im Bereich  MIN / Max Position
        //SetCommand();
    }
    void  UART:: handleSetTrack()
    {  // test Fehler 
        // Test  Busy
        // Refernziert ?
        // Test ob Gleis Kombi plausibel / aktuell
        //SetCommand();
        
    }
    void  UART:: handleSetRemote()
    {   


    }
    void  UART:: handleSetLocal()
    {
       // test Fehler 
        // Test  Busy
        // Refernziert ?
        // Test ob Lokales Befhels gerät da 
        //SetCommand();
      
    }

