
#include <Arduino.h>
#include <HardwareSerial.h>
#include "FY_System.h"
#include "Config.h"
#include "Protokoll.h"
#include "UART.h"

// daten typen der module 
#include "Motor.h"





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
/// @todo set job einbauen 



   UART:: UART()
   {
       //Init some Data 
       UART_context= nullptr;
       UART_Error = 0;
       uint8_t data[MAX_COMMAND_LENGTH - 1]{};

       commandLength =1;
       CommandBuffer.status = UART_CommandStatus_t::VALID;



   }
    
  
    ///@brief 
    /// @param context 
    void UART::begin(UART_Context_t& context,FY_ModuleContext_t& modules)
    {
         // Check if  content is set
         Serial.begin( UART_BAUD_RATE);
         this-> UART_context = &context;
         this-> FY_ModuleContext = &modules;

      // try to connect to the master e.g.ESP32
            sendHello();
            
             // what will be the Answer ? 
    }
    
    

    /* cyclic funktion to receive and send date from / to the master on serieal */
    void UART::update()
    {
     if ( this-> UART_context ==  nullptr)
             {       
            UART_Error |= UART_ERROR_NO_CONTENT;
            Serial.println(F("No content. The UART is feeling lonely."));
            return;
             }
    if (true == commandReady)
    return;
        receive();

        if (true == commandReady)
        {
            decodeCommand();
            sendResponse();

        commandReady = false; 
            // erst jetzt darf der nächste Command kommen
        
        }
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
        {case ReceiveState::FindCommand:
    // Read and validate a command byte when data is available.
    if (Serial.available())
    {
        uint8_t cmd = Serial.read();

        // Start a new command telegram.
        CommandBuffer.status = UART_CommandStatus_t::VALID;
        CommandBuffer.cmd = cmd;

        // Compare the received byte with all known command definitions.
        bool commandFound = false;

        for (uint8_t i = 0; i < COMMAND_COUNT; i++)
        {
            if (commandDefinitions[i].id == cmd)
            {
                CommandBuffer.type = commandDefinitions[i].type;
                CommandBuffer.response = commandDefinitions[i].response;

                // Store the expected payload length and prepare for data reception.
                expectedLength = commandDefinitions[i].telegramLength - 1;
                dataIndex = 0;
                lastByteTime = micros(); // Start für Timout überwachung 
                receiveState = ReceiveState::ReadData;

                commandFound = true;
                break;
            }
        }

        // No valid command was found.
        if (!commandFound)
        {
            CommandBuffer.status = UART_CommandStatus_t::CMD_INVALID;
        }
       }
       break;
       case ReceiveState::ReadData:
          if (Serial.available())
          {
              CommandBuffer.data[dataIndex++] = Serial.read();
      
              // A byte was received, so restart the inter-byte timeout.
              lastByteTime = micros();
      
              // All expected data bytes received.
              if (dataIndex >= expectedLength)
              {
                  receiveState = ReceiveState::CheckCRC;
              }
          }
          else if (micros() - lastByteTime > UART_TIMEOUT_US)
          {
              // Telegram incomplete.
              CommandBuffer.status = UART_CommandStatus_t::DATA_INVALID;
              receiveState = ReceiveState::FindCommand;
          }
        break;
     
        case ReceiveState::CheckCRC:
           if (Serial.available())
           {
               CommandBuffer.CMD_CRC = Serial.read();
       
               // CRC byte received.
               lastByteTime = micros();
       
               if (Check_CRC())
               {
                   CommandBuffer.status = UART_CommandStatus_t::VALID;
                   commandReady =true; 
               }
               else
               {
                   CommandBuffer.status = UART_CommandStatus_t::CRC_INVALID;
               }
       
               receiveState = ReceiveState::FindCommand;
           }
           else if (micros() - lastByteTime > UART_TIMEOUT_US)
           {
               // CRC byte missing.
               CommandBuffer.status = UART_CommandStatus_t::CRC_INVALID;
               receiveState = ReceiveState::FindCommand;
           }
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
          CommandBuffer.status = UART_CommandStatus_t::CMD_INVALID;
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
                 handle_Busy();
                 return;
             }


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
                CommandBuffer.status = UART_CommandStatus_t::CMD_INVALID;
                break;
            }
     }
    
     
    void UART::decodePriority()
    {
        switch (CommandBuffer.cmd)
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
     { MotorJob_t job{};

        // Das modul Motor  muss bekannt wein  sonst wirds  nix  
        if (FY_ModuleContext == nullptr || FY_ModuleContext->motor == nullptr)
         return false;

    

    job.cmd = CommandBuffer.cmd;

    for (uint8_t i = 0; i < MAX_COMMAND_LENGTH - 1; i++)
        job.data[i] = CommandBuffer.data[i];

    return (FY_ModuleContext ->motor->setJob(job));
    }
     




     ///@brief 
     bool UART::Check_CRC()
{
   #ifdef UART_USE_RX_CRC
    uint8_t crc = Calc_CRC(
        CommandBuffer.cmd,
        CommandBuffer.data,
        expectedLength
    );

    return crc == CommandBuffer.CMD_CRC;
    #else
    return true ;
    #endif

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
            {
                crc = (crc << 1) ^ 0x07;
            }
            else
            {
                crc <<= 1;
            }
        }
    }

    return crc;
}
     ///@brief  Errorhandling 
         UART_CommandStatus_t UART::getCommandStatus() 
    {
        return CommandBuffer.status;
    }
    
    void UART::clearCommandStatus()
    {
        CommandBuffer.status = UART_CommandStatus_t::VALID;
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
         setResponse(STATUS_System,reinterpret_cast<uint8_t*>(UART_context->systemStatus-> error),sizeof(FY_SystemState_t));
    }
 void UART::handleGetPosition()
    {
     setResponse(
         STATUS_Position, reinterpret_cast<uint8_t*>(UART_context->motorPosition),sizeof(int32_t));

    }
    void  UART:: handleGetTrack()
    {
        setResponse(STATUS_Track,reinterpret_cast<uint8_t*>(UART_context->Track_INFO),1);
        
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
    {  // CMD | Tracknummer | (CRC)
        
        // test SYSTEM Fehler 
        // Test MOTOR Busy
        // SYSTEM Refernziert 
        // (CRC OK ) 
        // CommandBuffer.data[0] == UART_context->Track_INFO ->akt_track !! type casten 
                
        if (( FY_Track::BG1 < CommandBuffer.data[0] )
          &&( FY_Track::BG5 > CommandBuffer.data[0]))
          {
           
          }
   
        
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

