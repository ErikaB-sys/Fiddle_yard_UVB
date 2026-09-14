// Motor.cpp
// Motor control implementation

#include <Arduino.h>
#include "Motor.h"


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
    // -> Moving  

    //or config data
    // -> Config 

        break;
    case CONF :
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

void Motor::move2Pos(long targetPosition)
{
    // TODO:
    // - check reference state
    // - check target limits
    // - store target position
    // - calculate movement profile
    // - start movement
}

void Motor::moveSteps(long Steps)
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