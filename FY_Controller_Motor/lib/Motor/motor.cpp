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
// Bewegungs und ISR Daten 

    // Dieses Motor-Objekt ist dasjenige, das Timer1 verwendet.
    _TimerMotor = this;

    Timer1_Init();// prepair timer 1  for creating  Steps 
    Timer1_Stop(); // dont move!!

    // Daten 
    _State = MotorState_t::IDLE;
    _ProfileElement = MotorProfile_t::ACC1;
    _StepsRemaining = 0;
    _TimerValue     = 0;
    _Position       = 0;
    _TargetPosition = 0;






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
                 break;       
             case CMD_RIGHT:
             if ( true == prepareRight())
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


bool setJob(MotorJob_t newjob)

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

      
    return (true);

   };
   bool Motor::prepareParameter()
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
// ================================================================
// Fahrprofil:
//
// Geschwindigkeit
//      ^
// Vmax |                 +-------------+
//      |              /                 \
// Vmin |-------------+                   +----------------
//      +--------------------------------------------------> Weg
//             ACC1   ACC2    KONST      BRE1   BRE2   POSI
//             <----> <---->  <--------> <----> <----> <-->
//              Steps  Steps      Steps    Steps  Steps  Steps
//
// ACC1 / ACC2 : Beschleunigung
// KONST       : Fahrt mit V_MAX
// BRE1 / BRE2 : Bremsen
// POSI        : Restweg mit V_MIN
//
// ACC1 == BRE2
// ACC2 == BRE1
//
// Summe aller Steps == angeforderte Strecke
// ================================================================

 bool Motor::calcProfile(uint16_t distance)
{
    // -------------------------------------------------
    // Profil zunächst vollständig löschen
    // -------------------------------------------------

    for (uint8_t i = 0; i < 6; i++)
    {
        Motor_profil[i].Steps = 0;
        Motor_profil[i].Accel = 0;
    }


    // -------------------------------------------------
    // Sehr kurze Strecke:
    // Kein vollständiges Profil möglich.
    // Die gesamte Strecke wird mit V_MIN gefahren.
    // -------------------------------------------------

    if (distance <= (_ProfileParam.PosiMin +
                     _ProfileParam.KonstMin))
    {
        Motor::Motor_profil[static_cast<uint8_t>(MotorProfile_t::KONST)].Steps =
            distance;

        return true;
    }


    // -------------------------------------------------
    // POSI und KONST_MIN reservieren
    // -------------------------------------------------

    Motor::Motor_profil[static_cast<uint8_t>(MotorProfile_t::POSI)].Steps =
        _ProfileParam.PosiMin;

    Motor::Motor_profil[static_cast<uint8_t>(MotorProfile_t::KONST)].Steps =
        _ProfileParam.KonstMin;

    uint16_t remaining =
        distance
        - _ProfileParam.PosiMin
        - _ProfileParam.KonstMin;


    // -------------------------------------------------
    // ACC1 / BRE2
    //
    // Beide Bereiche werden immer gleich groß.
    // Integer-Division verhindert Float-Rechnung.
    // -------------------------------------------------

    uint16_t outer = remaining / 2;

    if (outer > _ProfileParam.Acc1Steps)
        outer = _ProfileParam.Acc1Steps;

    Motor_profil[static_cast<uint8_t>(MotorProfile_t::ACC1)].Steps =
        outer;

    Motor_profil[static_cast<uint8_t>(MotorProfile_t::ACC1)].Accel =
       _ProfileParam.Acc1Accel;

    Motor_profil[static_cast<uint8_t>(MotorProfile_t::BRE2)].Steps =
        outer;

    Motor_profil[static_cast<uint8_t>(MotorProfile_t::BRE2)].Accel =
        _ProfileParam.Bre2Accel;

    remaining -= outer * 2;


    // -------------------------------------------------
    // ACC2 / BRE1
    // -------------------------------------------------

    uint16_t inner = remaining / 2;

    if (inner > _ProfileParam.Acc2Steps)
        inner = _ProfileParam.Acc2Steps;

    Motor_profil[static_cast<uint8_t>(MotorProfile_t::ACC2)].Steps =
        inner;

    Motor_profil[static_cast<uint8_t>(MotorProfile_t::ACC2)].Accel =
        _ProfileParam.Acc2Accel;

    Motor_profil[static_cast<uint8_t>(MotorProfile_t::BRE1)].Steps =
        inner;

    Motor_profil[static_cast<uint8_t>(MotorProfile_t::BRE1)].Accel =
        _ProfileParam.Bre1Accel;

    remaining -= inner * 2;


    // -------------------------------------------------
    // Rest geht in KONST
    // -------------------------------------------------

    Motor_profil[static_cast<uint8_t>(MotorProfile_t::KONST)].Steps +=
        remaining;


    // -------------------------------------------------
    // Sicherheit / Quantisierungsrest
    //
    // Die Summe aller Profilbereiche muss exakt
    // der angeforderten Strecke entsprechen.
    // Falls durch Integer-Division ein Rest entstanden
    // ist, landet er in POSI.
    // -------------------------------------------------

    uint16_t sum = 0;

    for (uint8_t i = 0; i < 6; i++)
        sum += Motor_profil[i].Steps;

    if (sum < distance)
    {
        Motor_profil[static_cast<uint8_t>(MotorProfile_t::POSI)].Steps +=
            distance - sum;
    }
_TimerValid = true;
    return true;
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
        // Profil immer am Anfang beginnen
    _ProfileElement = MotorProfile_t::ACC1;

    // Anzahl der Schritte des ersten Profilelements laden
    _StepsRemaining =
        Motor_profil[static_cast<uint8_t>(_ProfileElement)].Steps;
        // timer einstellen 
      _TimerValue= TIMER_V_MIN;
       OCR1A = _TimerValue;
       // und ab geht die wilde  Fahrt .....
    TCCR1B |= (1 << CS10);
}

void Motor::Timer1_Stop()
{
    TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));
}

void Motor::Timer1_ISR()
{
    // ------------------------------------------------------------
    // Timer1_ISR() wird direkt vom AVR-Interrupt aufgerufen.
    //
    // Da die Funktion static ist, gibt es hier kein "this".
    // _TimerMotor zeigt deshalb auf das Motor-Objekt, dessen
    // Timerdaten wir bearbeiten müssen.
    // ------------------------------------------------------------

    Motor* motor = _TimerMotor;

    // Sicherheitsprüfung:
    // Falls noch kein Motor-Objekt mit Timer1 verbunden wurde,
    // gibt es hier nichts zu tun.
    if (motor == nullptr)
    {
        return;
    }

    if (!motor->_TimerValid)
    {
        return;
    }

    // ------------------------------------------------------------
    // Hier arbeitet die ISR jetzt ganz normal mit den Membern
    // des Motor-Objektes.
    // ------------------------------------------------------------

    static bool ticktack = false;

    ticktack = !ticktack;

    if (ticktack)
    {
        // STEP HIGH

    }
    else
    {
        // STEP LOW

        // Ein vollständiger STEP ist abgeschlossen.
        motor->_StepsRemaining--;

      if (motor->_StepsRemaining == 0)
          {
           // Zum nächsten Profilelement wechseln
          for (uint8_t i = 0; i < 6; i++)
           {
             // nächstes Profilelement bestimmen
             switch (motor->_ProfileElement)
               {
                 case MotorProfile_t::ACC1:
                     motor->_ProfileElement = MotorProfile_t::ACC2;
                     break;
         
                 case MotorProfile_t::ACC2:
                     motor->_ProfileElement = MotorProfile_t::KONST;
                     break;
         
                 case MotorProfile_t::KONST:
                     motor->_ProfileElement = MotorProfile_t::BRE1;
                     break;
         
                 case MotorProfile_t::BRE1:
                     motor->_ProfileElement = MotorProfile_t::BRE2;
                     break;
         
                 case MotorProfile_t::BRE2:
                     motor->_ProfileElement = MotorProfile_t::POSI;
                     break;
         
                 case MotorProfile_t::POSI:
                     // Kein weiteres Profilelement vorhanden.
                     motor->_TimerValid = false;
                     motor->Timer1_Stop();
                     return;
                }
   
             // Schritte des neuen Profilelements laden
             uint8_t element =
                 static_cast<uint8_t>(motor->_ProfileElement);
         
             motor->_StepsRemaining =
                 motor->Motor_profil[element].Steps;
         
                // Profilelement gefunden
              if (motor->_StepsRemaining > 0)
                {
                    break;
                }
            }
            // Jetzt steht fest, welches Profilelement für
            // den NÄCHSTEN Schritt gilt.
            
            motor->_TimerValue +=
                motor->Motor_profil[
                    static_cast<uint8_t>(motor->_ProfileElement)
                ].Accel;
            
            // Timerwert für den nächsten Interrupt übernehmen.
            OCR1A = motor->_TimerValue;
      
        }
     }
}





ISR(TIMER1_COMPA_vect)
{/// @brief  Compare

Motor::Timer1_ISR();

}
