#pragma once
// Motor.h
// Declarations for motor control used by the project.

enum class MotorState_t
{
    IDLE,       // Motor is idle.
    CONF,       // Change of motor & moving parameter
    MOVING,     // Motor is moving.
    STOPPED,    // Motor has been stopped.
    ERROR       // Motor is in an error state.
};






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
    bool motor_isMoving();
    uint16_t motor_getPosition();
    uint16_t motor_getTargetPosition();
    MotorState_t motor_getState();
    



private:
  // Hardware
    uint8_t _Dir_pin;
    uint8_t _Step_pin;
    uint8_t _ENA_pin;

    // Position
    uint16_t _Position;
    uint16_t _TargetPosition;
    struct MotorJob_t
        {
            uint8_t      cmd;
            uint8_t      data[MAX_COMMAND_LENGTH - 1];
        
        };
    MotorJob_t _Job;

   


    // State
    MotorState_t _State;

    // Movement profile
  
    uint8_t _ProfileElement;


   

   // Prüfungen der Daten  
   bool prepareSetPosition();
   bool prepareLeft();
   bool prepareRight();
   bool prepareSetTrack();
   bool CheckBorder();
   bool calcProfile();
   bool prepareParameter();
   bool setJob(Motor::MotorJob_t newjob);

   
    // TODO: timing / step counter
    // Calculate the movement profile.
     bool calcprofil( );

    // ----------------------------------------------------------------
    // Timer 1
    // ----------------------------------------------------------------
    void Timer1_Init();
    void Timer1_Start();
    void Timer1_Stop();

    //------------------------------------------------------------------------------------
    /// @defgroup Fahrprofil 
    //------------------------------------------------------------------------------------
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

   volatile MotorProfile_t _ProfileElement = MotorProfile_t::ACC1;
   volatile uint16_t _StepsRemaining = 0;
   volatile uint16_t _TimerValue = 0;

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


  