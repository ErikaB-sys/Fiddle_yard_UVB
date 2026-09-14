// Motor.cpp
// Motor control implementation

#include <Arduino.h>
#include "Motor.h"
#include "Protokoll.h"



// -----------------------------------------------------------------------------
// Constructor / Destructor
// -----------------------------------------------------------------------------

Motor::Motor()
{
    // Initial state
    // TODO
}



// -----------------------------------------------------------------------------
// Initialization
// -----------------------------------------------------------------------------

bool Motor::begin(uint8_t Dir_pin, uint8_t Step_pin, uint8_t ENA_pin)
{
    // TODO:
    // - store pin numbers

  // Treiber deaktiviert
    // - configure pins
    // - set initial output states
    // - configure Timer2

    _Dir_pin  = Dir_pin;
    _Step_pin = Step_pin;
    _ENA_pin  = ENA_pin;

    pinMode(_Dir_pin, OUTPUT);
    pinMode(_Step_pin, OUTPUT);
    pinMode(_ENA_pin, OUTPUT);

    digitalWrite(_Step_pin, LOW);
    digitalWrite(_Dir_pin, LOW);

    // Motor initially disabled
    digitalWrite(_ENA_pin, HIGH);

    _Position       = 0;
    _TargetPosition = 0;

    _State = IDLE;

    _ProfileElement = 0;
    _StepsRemaining = 0;
    _TimerValue     = 0;

    return true;
}



// -----------------------------------------------------------------------------
// Main update function
// -----------------------------------------------------------------------------

void Motor::Update()
{
    // Called cyclically from loop()

    // TODO:
    switch (_State)
    {
    case IDLE :
    // wait  for Job 
    
             switch (cmd)
             { // all checks  -> Moving 
             case CMD_REFERENCE:
                  
             case CMD_LEFT:
                  if ( true == prepareLeft())
                  {     _State = MOVING;             }
                  else 
                  { // Error setzen 
                      _State = ERROR; }
             case CMD_RIGHT:
             if ( true == prepareLeft())
                  {     _State = MOVING;             }
                  else 
                  { //pos  Error setzen 
                      _State = ERROR; }

             case CMD_SET_POSITION:
                   if (true == prepareSetPosition())
                   {     _State = MOVING;             }
                  else 
                  { //pos  Error setzen 
                      _State = ERROR; }
             break;      
             case CMD_SET_TRACK: 
                   if (true == prepareSetTrack())
                   {     _State = MOVING;             }
                  else 
                  { //pos  Error setzen 
                      _State = ERROR; }
                break;
             case CMD_SET_SPEED : 
              //or config data
               // -> Config 
               if  (true ==  prepareParameter())
                   {     _State = CONF;             }
                  else 
                  { //pos  Error setzen 
                      _State = ERROR; }
             break;

             default:
               _State = ERROR; 
                break;
             }
    
   
  
        break;
    case CONF :
      // Parameter  ist in range ?
      if (setParameter())
      {
         _State = IDLE;
      };

     //-> Idle 

    break;
    case MOVING :
        // while  moving stay 
        // pos reached 
        // or Stopp cmd --> Stopped 

    break;
    case STOPPED :
       // New  Job ? 
       // Y -> moving 
        // n -> IDLE
    break;
    case  ERROR  :
    
    break;
    default:
     _State = ERROR;

        break;
    
    } //end  of Switch motorstate 
    // - evaluate current motor state
    // - start / continue / finish movement
    // - handle profile changes
}


// -----------------------------------------------------------------------------
// Movement commands
// -----------------------------------------------------------------------------

void Motor::move2Pos(int16_t targetPosition)
{
    // TODO:
    // - check reference state
    // - check target limits
    // - store target position
    // - calculate movement profile
    // - start movement
}

void Motor::moveSteps(int16_t Steps)
{
    // TODO:
    // - calculate target from current position
    // - no reference required
    // - calculate movement profile
    // - start movement
}


// -----------------------------------------------------------------------------
// Stop / emergency
// -----------------------------------------------------------------------------

void Motor::Stop()
{
    // TODO:
    // - stop generating STEP pulses
    // - keep motor enabled
    // - update state
}

void Motor::Emergency_break()
{
    // TODO:
    // - immediately disable motor
    // - stop Timer2 / STEP generation
    // - update state
}


// -----------------------------------------------------------------------------
// Reference
// -----------------------------------------------------------------------------

bool Motor::Reference()
{
    // TODO:
    // - check whether reference run can be started
    // - start reference movement

    return false;
}
//-----------------------------------------------------------------------------
//   Helping functions 
//-----------------------------------------------------------------------------


   bool Motor::prepareSetPosition()
   {
    // refernziert ?   -> missing ref Error 

        // position innerhalb ?  --> out of range Error 

    // calcprofil() ; 
     return (true)

   };
   bool Motor::prepareLeft()
   {
    // refernziert ?   -> missing ref Error   
     // akt pos  + TrackSTEP ( ein gleis ? ) 
         // position innerhalb ?  --> out of range Error 

    // calcprofil() ; 
     return (true)  
   };
   bool Motor::prepareRight()
   {
      // refernziert ?   -> missing ref Error   
     // akt pos  +  TrackSTEP ( ein gleis ? ) 
         // position innerhalb ?  --> out of range Error 

    // calcprofil() ; 
     return (true) 
   };
   bool Motor::prepareSetTrack()
   {
     // refernziert ?   -> missing ref Error  
         // TRACK innerhalb ?  --> TRACKout of range Error  
     // Track2Pos ( );
        
    // calcprofil() ; 
     return (true);

   };
   bool Motor::CheckBorder()
   {
      
    
   };
   bool prepareParameter()
   {
        // valider parameter?  --> invalidparamError 
        // parameter in range   --> OORParamError 
         return (true);
        
   };

// -----------------------------------------------------------------------------
// Status / information
// -----------------------------------------------------------------------------

bool Motor::motor_isMoving()
{
    // TODO
    return false;
}

uint16_t Motor::motor_getPosition()
{
    // TODO
    return 0;
}

uint16_t Motor::motor_getTargetPosition()
{
    // TODO
    return 0;
}

MotorState_t Motor::motor_getState()
{
    // TODO
    return IDLE;
}


// -----------------------------------------------------------------------------
// Movement profile
// -----------------------------------------------------------------------------

bool Motor::calcprofil()
{
    // TODO:
    // Calculate movement profile:
    //
    // ACC1
    // ACC2
    // KONST
    // BRE1
    // BRE2
    // POSI

    return false;
}


// -----------------------------------------------------------------------------
// Timer2 interrupt
// -----------------------------------------------------------------------------

ISR(TIMER2_OVF_vect)
{
    // TODO:
    // Generate STEP pulse according to current profile element.
    //
    // IMPORTANT:
    // Keep ISR as short as possible.
}
