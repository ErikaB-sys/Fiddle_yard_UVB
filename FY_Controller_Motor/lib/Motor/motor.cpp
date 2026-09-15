// Motor.cpp
// Motor control implementation

#include <Arduino.h>
#include "FY_System.h"
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

    _State = MotorState_t::IDLE;

    _ProfileElement = MotorProfile_t::ACC1;
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
    case MotorState_t::IDLE :
    // wait  for Job 
    
             switch (_Job.cmd)
             { // all checks  -> Moving 
             case CMD_GO:
                  //Timer1_Start();
              break ;   

             case CMD_REFERENCE:

                  break;
             case CMD_LEFT:
                  if ( true == prepareLeft())
                  {     _State = MotorState_t::MOVING;             }
                  else 
                  { // Error setzen 
                      _State = MotorState_t::ERROR; }
             case CMD_RIGHT:
             if ( true == prepareLeft())
                  {     _State = MotorState_t::MOVING;             }
                  else 
                  { //pos  Error setzen 
                      _State = MotorState_t::ERROR; }

             case CMD_SET_POSITION:
                   if (true == prepareSetPosition())
                   {     _State = MotorState_t::MOVING;             }
                  else 
                  { //pos  Error setzen 
                      _State =MotorState_t:: ERROR; }
             break;      
             case CMD_SET_TRACK: 
                   if (true == prepareSetTrack())
                   {     _State = MotorState_t:: MOVING;             }
                  else 
                  { //pos  Error setzen 
                      _State = MotorState_t::ERROR; }
                break;
             case CMD_SET_SPEED : 
              //or config data
               // -> Config 
               if  (true ==  prepareParameter())
                   {     _State = MotorState_t::CONF;             }
                  else 
                  { //pos  Error setzen 
                      _State = MotorState_t::ERROR; }
             break;

             default:
               _State = MotorState_t::ERROR; 
                break;
             }
    
   
  
        break;
    case MotorState_t::CONF :
      // Parameter  ist in range ?
      if (prepareParameter())
      {
         _State = MotorState_t::IDLE;
      };

     //-> Idle 

    break;
    case MotorState_t::MOVING :
        // while  moving stay 
        // pos reached 
        // or Stopp cmd --> Stopped 

    break;
    case MotorState_t::STOPPED :
       // New  Job ? 
       // Y -> moving 
        // n -> IDLE
    break;
    case  MotorState_t::ERROR  :
    
    break;
    default:
     _State = MotorState_t::ERROR;

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


bool Motor::setJob(Motor::MotorJob_t newjob)

{
    if (_State != MotorState_t::IDLE && _State != MotorState_t::STOPPED)
        return false;

    _Job = newjob;       // bewusst kopieren
    return true;
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
    digitalWrite(_ENA_pin , HIGH);
    
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
     return (true);

   }
   bool Motor::prepareLeft()
   {
    // refernziert ?   -> missing ref Error   
     // akt pos  + TrackSTEP ( ein gleis ? ) 
         // position innerhalb ?  --> out of range Error 

    // calcprofil() ; 
     return (true);  
   }


   bool Motor::prepareRight()
   {
      // refernziert ?   -> missing ref Error   
     // akt pos  +  TrackSTEP ( ein gleis ? ) 
         // position innerhalb ?  --> out of range Error 

    // calcprofil() ; 
     return (true) ;
   }
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
   
   // start fkt !!!

// -----------------------------------------------------------------------------
// Status / information
// -----------------------------------------------------------------------------

bool Motor::motor_isMoving()
{
    if (MotorState_t::MOVING == _State)
       {return true;}
       else
       {
    return false;
       }
}

uint16_t Motor::motor_getPosition()
{
 // ggf  den zähler aus demIntrrupt mit kurzer Interrupt Sperre  holen

    return _Position;
}

uint16_t Motor::motor_getTargetPosition()
{
  
    return   _TargetPosition;
}

MotorState_t Motor::motor_getState()
{
    
    return _State;
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
// Timer1 interrupt
// -----------------------------------------------------------------------------
void Motor::Timer1_Init()
{
    // Timer 1 stoppen und konfigurieren
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1  = 0;

    // CTC-Modus: TOP = OCR1A
    TCCR1B |= (1 << WGM12);

    // Compare-Match-A-Interrupt aktivieren
    TIMSK1 |= (1 << OCIE1A);

    // Startwert
    OCR1A = 1000;

    // Prescaler = 1
    TCCR1B |= (1 << CS10);
}

void Motor::Timer1_Start()
{
    TCCR1B |= (1 << CS10);
}

void Motor::Timer1_Stop()
{
    TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));
}


ISR(TIMER1_COMPA_vect)
{/// @brief  Compare
/*      ↓
ISR
 ├─ Valid prüfen
 ├─ STEP erzeugen
 ├─ Schritt zählen
 ├─ StepsRemaining--
 ├─ Timerwert anpassen
 ├─ ggf. nächstes Profilelement
 └─ bei Ende → Valid = false / Timer stoppen
 */

}
