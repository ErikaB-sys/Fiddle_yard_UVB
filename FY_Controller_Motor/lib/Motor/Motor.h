#pragma once
// Motor.h
//default declarations 


// -----------------------------------------------------------------------------
// Mechanik
// -----------------------------------------------------------------------------

// mm  je Umdrehung typischer  20mm 
#ifndef MM_PER_UMD
#define MM_PER_UMD 20.0
#endif

#ifndef STEPS_PER_UMD
#define STEPS_PER_UMD 1600
#endif

// Profil default werte 
constexpr uint16_t MM_TO_STEPS(float mm)
{
    return static_cast<uint16_t>(
        (mm * STEPS_PER_UMD) / MM_PER_UMD
    );
}

#ifndef POS_MIN
constexpr uint16_t POS_MIN = MM_TO_STEPS(2.0);
#endif
#ifndef KONST_MIN
constexpr uint16_t KONST_MIN = MM_TO_STEPS(5.0);

#endif 




static constexpr uint16_t POSITION_UNKNOWN = UINT16_MAX / 2;
// -----------------------------------------------------------------------------
// Geschwindigkeit
// -----------------------------------------------------------------------------




#define TIMER_FREQUENCY 16000000UL
#define TIMER_PRESCALER 1

#define SPEED_TO_TIMER(v) \
    (TIMER_FREQUENCY / (TIMER_PRESCALER * 2UL * \
    ((v) * STEPS_PER_UMD / MM_PER_UMD)))

constexpr uint16_t V_MIN = 1;
constexpr uint16_t V_MAX = 50;

constexpr uint16_t TIMER_V_MIN = static_cast<uint16_t>(SPEED_TO_TIMER(V_MIN));

constexpr uint16_t TIMER_V_MAX = static_cast<uint16_t>(SPEED_TO_TIMER(V_MAX));




struct MotorJob_t
        {   bool         valid;
            uint8_t      cmd;
            uint8_t      data[MAX_COMMAND_LENGTH - 1];
        
        };
MotorJob_t _Job;


// Declarations for motor control used by the project.

enum class MotorState_t
{
    IDLE,       // Motor is idle.
    CONF,       // Change of motor & moving parameter
    REFRUN,     // Refrun as  new state 
    MOVING,     // Motor is moving.
    STOPPED,    // Motor has been stopped.
    ERROR       // Motor is in an error state.
};


// -----------------------------------------------------------------------------
// Fahrprofil-Parameter
// -----------------------------------------------------------------------------



enum  class MotorProfile_t
{
    ACC1,      // Start acceleration.
    ACC2,      // Increase speed.
    KONST,     // Move at constant speed.
    BRE1,      // Brake from constant speed.
    BRE2,      // Brake to positioning speed.
    POSI       // Find the position at very low speed.

};

/// @concept  Accel < 0 → Timerwert wird kleiner → schneller
//            Accel = 0 → konstante Geschwindigkeit
//            Accel > 0 → Timerwert wird größer → langsamer
//            Steps ist die anzahl der schritte mit derjeweiligen Beschleinigung
struct Profilelement_t
{
    uint16_t Steps; // Number of steps in this profile element.
    int16_t Accel;  // Acceleration for this profile element.!! vorzeichen behaftet !!!
};
struct MotorProfileParam_t
{  
    uint16_t PosiMin ;
    uint16_t KonstMin;
 
    uint16_t Acc1Steps;
    uint16_t Acc2Steps;

    int16_t Acc1Accel;
    int16_t Acc2Accel;
    int16_t Bre1Accel;
    int16_t Bre2Accel;

};

//default Parameter 

MotorProfileParam_t _ProfileParam =
{
    MM_TO_STEPS(POS_MIN),     // PosiMin
    MM_TO_STEPS(KONST_MIN), // KonstMin

    100,                    // Acc1Steps
    100,                    // Acc2Steps

    -2,                     // Acc1Accel
    -1,                     // Acc2Accel
    +1,                     // Bre1Accel
    +2                      // Bre2Accel
};

enum class MotorJobResult_t
{
    NONE,
    DONE,           // Profil vollständig gefahren
    ENDSTOP_LEFT,   // durch linken Endschalter beendet
    ENDSTOP_RIGHT   // durch rechten Endschalter beendet
};


class Motor
{
public:
    Motor();
    // Isr is member  of motor ....
    static void Timer1_ISR();

    // Initialize the motor control pins.
    bool begin(uint8_t Dir_pin, uint8_t Step_pin, uint8_t ENA_pin);
    
    // Update the motor control state.
    void Update();

    bool  setJob(MotorJob_t newjob); // set Job after the condition was  checked  extern....!!

    // Start a move to the target position. Requires the motor to be referenced.
    void move2Pos(int16_t targetPosition);

    // Move the specified number of steps. Referencing is not required.
    void moveSteps(int16_t Steps);

    // Immediately disable the motor using the enable pin.
    void Emergency_break();

    // Stop the current movement.
    void Stop();

    // Start a reference run and return whether it was started successfully.
    bool Reference();

    // Return motor information.
    bool             isMoving();
    uint16_t         getPosition();
    MotorJobResult_t getJobResult() const;
  
    uint16_t motor_getTargetPosition();
    MotorState_t motor_getState();
    



private:
  // Hardware
    uint8_t _Dir_pin;
    uint8_t _Step_pin;
    uint8_t _ENA_pin;


    // State
    MotorState_t _State;

    // Movement profile
  
   volatile  MotorProfile_t  _ProfileElement;  // Akives Element 

    // Position

    uint16_t _TargetPosition;
    // Results of Movment 
    MotorJobResult_t _JobResult;
    uint32_t _Position;    // ! _position  ist immer  Positiv 

   // Prüfungen der Daten  
   bool prepareSetPosition();
   bool prepareLeft();
   bool prepareRight();
   bool prepareSetTrack();
   bool CheckBorder();
   bool prepareParameter();
   

   
    // TODO: timing / step counter
    // Calculate the movement profile.
     bool calcProfile(uint16_t distance );

    // ----------------------------------------------------------------
    // Timer 1
    // ----------------------------------------------------------------
    void Timer1_Init();
    void Timer1_Start();
    void Timer1_Stop();

    //------------------------------------------------------------------------------------
    /// @defgroup Fahrprofil 
    //------------------------------------------------------------------------------------
    /*                      Motorprofil 
                         V
                         ^
                         |
                    Vmax |              ┌───────────────┐
                         |            /                  \
                         |          /                      \
                    Vmin |─────────┘                        └──────────
                         |
                         +------------------------------------------------> Weg
                           |       |       |            |       |       |
                           | ACC1  | ACC2  |   KONST    | BRE1  | BRE2  | POSI
                           |       |       |            |       |       |
                           <------> <------> <----------> <------> <------> <---->
                             Steps   Steps     Steps       Steps   Steps    Steps
   
   */
    volatile bool _TimerValid = false;

    // Motorprofil: ACC1, ACC2, KONST, BRE1, BRE2, POSI
    // testtabelle als default wert 
    Profilelement_t Motor_profil[6]=
    {
    {100, -2},   // ACC1
    {100, -1},   // ACC2
    {200,  0},   // KONST
    {100, +1},   // BRE1
    {100, +2},   // BRE2
    { 10,  0}    // POSI
    };


   volatile uint16_t _StepsRemaining = 0;
   volatile uint16_t _TimerValue = 0;
   
    // ------------------------------------------------------------
    // Verbindung zwischen Hardware-ISR und Motor-Objekt
    // ------------------------------------------------------------
    //
    // Timer1_ISR() ist static und besitzt deshalb kein "this".
    // Über diesen Zeiger findet sie das eine Motor-Objekt,
    // dessen Timer1 sie bedienen soll.
    //
    static Motor* _TimerMotor;

    // ----------------------------------------------------------------
    // Bewegungsgrenzen
    // ----------------------------------------------------------------

    // Mechanischer Bewegungsbereich in Schritten
    static constexpr uint16_t POS_MIN = 0;
    static constexpr uint16_t POS_MAX = 6400;

    // Timergrenzen
    // Kleinerer Wert = höhere Geschwindigkeit
    static constexpr uint16_t TIMER_MIN = 100;
    static constexpr uint16_t TIMER_MAX = 30000;

    // Timeränderung pro Schritt
    // negativ = schneller
    // positiv = langsamer
    static constexpr int16_t ACCEL_MIN = -100;
    static constexpr int16_t ACCEL_MAX = 100;

};


  