#pragma once
// Motor.h
//default declarations 

#include "FY_System.h"
#include "Protokoll.h"
#include "Error.h"

// -----------------------------------------------------------------------------
// Mechanik
// -----------------------------------------------------------------------------

// mm  je Umdrehung typischer  20mm 
#ifndef MM_PER_UMD
#define MM_PER_UMD 20.0
#endif

#ifndef STEPS_PER_UMD
// Verifiziert mit FY_HW_TEST: 200 Vollschritte/U * 1/4 Microstepping
#define STEPS_PER_UMD 800
#endif

// Profil default werte 
constexpr uint16_t MM_TO_STEPS(float mm)
{
    return static_cast<uint16_t>(
        (mm * STEPS_PER_UMD) / MM_PER_UMD
    );
}

#ifndef POS_MIN
constexpr uint16_t POS_MIN = 160;
#endif
#ifndef KONST_MIN
constexpr uint16_t KONST_MIN = 400;
#endif 


static constexpr uint16_t POSITION_UNKNOWN = UINT16_MAX / 2;
// -----------------------------------------------------------------------------
// Geschwindigkeit
// -----------------------------------------------------------------------------

#define TIMER_FREQUENCY 16000000UL
// Match the verified FY_HW_TEST Timer1 prescaler (/256).
#define TIMER_PRESCALER 256UL

constexpr uint16_t V_MIN = 120;
constexpr uint16_t V_MAX = 800;

// Convert STEP frequency (Hz) to the Timer1 CTC compare value.
// Round to the nearest timer count, then subtract one for OCR1A.
constexpr uint16_t speed_to_timer(uint16_t frequency)
{
    return static_cast<uint16_t>(
        ((TIMER_FREQUENCY +
          ((TIMER_PRESCALER * 2UL * frequency) / 2UL)) /
         (TIMER_PRESCALER * 2UL * frequency)) - 1UL);
}

constexpr uint16_t TIMER_V_MIN = speed_to_timer(V_MIN);
constexpr uint16_t TIMER_V_MAX = speed_to_timer(V_MAX);


struct MotorJob_t
        {   bool         valid;
            uint8_t      cmd;
            uint8_t      data[MAX_COMMAND_LENGTH - 1];
        
        };
// Motor error definitions belong to the Motor module.
// Error stores and manages the current error state centrally.
constexpr uint8_t MOTOR_ERROR_NOT_ACTIVE      =
    FY_ERROR_LOCATION_MOTOR | 0x01U;
constexpr uint8_t MOTOR_ERROR_TARGET_RANGE     =
    FY_ERROR_LOCATION_MOTOR | 0x02U;
constexpr uint8_t MOTOR_ERROR_INVALID_TRACK   =
    FY_ERROR_LOCATION_MOTOR | 0x03U;
constexpr uint8_t MOTOR_ERROR_INVALID_JOB     =
    FY_ERROR_LOCATION_MOTOR | 0x04U;
constexpr uint8_t MOTOR_ERROR_INVALID_PARAM   =
    FY_ERROR_LOCATION_MOTOR | 0x05U;
constexpr uint8_t MOTOR_ERROR_INVALID_PROFILE =
    FY_ERROR_LOCATION_MOTOR | 0x06U;
constexpr uint8_t MOTOR_ERROR_PROFILE_FAILED  =
    FY_ERROR_LOCATION_MOTOR | 0x07U;

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

    // Unabhängige Rampenintervalle, verifiziert in FY_HW_TEST.
    uint16_t Acc1RampInterval;
    uint16_t Acc2RampInterval;
    uint16_t Bre1RampInterval;
    uint16_t Bre2RampInterval;
};

enum class MotorCommandResult_t : uint8_t
{
    ACCEPTED,
    INVALID_DATA,
    STOP_ACTIVE,
    BUSY
};

enum class MotorJobResult_t
{
    NONE,
    DONE,           // Profil vollständig gefahren
    ENDSTOP_LEFT,   // durch linken Endschalter beendet
    ENDSTOP_RIGHT   // durch rechten Endschalter beendet
};



 // FY_HW_TEST motor execution state — step 3a.
 // The old Motor members remain temporarily until step 3b removes the parallel state.
enum class RefRunState : uint8_t
{
    IDLE, REF_START, REF_LEFT_END, REF_SEARCH_RIGHT, REF_RIGHT_END,
    REF_RETURN_TO_REF, REF_APPROACH_REF, REF_SLOW_REF, REF_MEASURE_RIGHT,
    REF_SHIFT_LEFT, REF_MEASURE_LEFT, REF_CALCULATE, REF_VALID, REF_ERROR
};

struct HWISRUiState { bool displayDirty{true}; };

extern volatile bool stepRun;
extern volatile bool stepLevel;
extern volatile bool profileRun;
extern volatile bool profileReportPending;
extern volatile bool updateOut;
extern HWISRUiState hw;
extern uint16_t profileVMin;
extern uint16_t profileVMax;
extern volatile uint8_t profileRampCounter;
extern Profilelement_t motionProfile[6];
extern volatile uint8_t profileElement;
extern volatile uint16_t profileStepsRemaining;
extern volatile uint16_t profileTimerValue;
extern volatile uint16_t profileMinReloadReached;

extern RefRunState refRunState;
extern bool refRunActive;
extern bool refLongRun;
extern uint16_t refRunVMax;
extern uint16_t refBackoffTarget;
extern volatile uint16_t refCoarsePosition;
extern volatile uint16_t refRightEndPosition;
extern volatile uint16_t refPreciseRight;
extern volatile uint16_t refPreciseLeft;
extern volatile uint16_t refPreciseLength;
extern volatile uint16_t refPreciseCenter;
extern volatile uint16_t refPreciseRight2;
extern volatile uint16_t refPreciseLeft2;
extern volatile uint16_t refPreciseLength2;
extern volatile uint16_t refPreciseCenter2;
extern volatile uint16_t refApproachTarget;
extern volatile uint16_t refShiftTarget;
extern volatile uint8_t refMeasurePass;
extern volatile bool refCoarseCaptured;
extern volatile bool refPreciseRightCaptured;
extern volatile bool refPreciseCaptured;
extern volatile bool refSecondPreciseCaptured;
extern volatile bool refMeasureArmed;
extern volatile bool refLastState;
extern volatile bool refStartValid;
extern volatile bool leftReferenceApplied;
extern volatile bool rightLengthCaptured;

bool load_next_profile_element();
void get_measurement(HWMeasurement* result);

class Motor
{
public:
    Motor();
    // Isr is member  of motor ....
    static void Timer1_ISR();

    // Initialize the motor control pins.
    bool begin();
    
    // Update the motor control state.
    void Update();

    bool  setJob(MotorJob_t newjob); // set Job after the condition was  checked  extern....!!
    // GO/STOPP are immediate motor-state commands; UART owns the response telegram.
    MotorCommandResult_t prepareGo(const uint8_t* data);
    MotorCommandResult_t prepareStop(const uint8_t* data);

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
    bool             isDriverActive() const;
    uint16_t         getPosition();
    MotorJobResult_t getJobResult() const;
  
    uint16_t motor_getTargetPosition();
    MotorState_t motor_getState();
    


private:
  // Hardware
    uint8_t _Dir_pin;
    uint8_t _Step_pin;
    uint8_t _ENA_pin;

    // Cached AVR port registers for timing-critical STEP/DIR access.
    volatile uint8_t* _StepPort = nullptr;
    volatile uint8_t* _DirPort  = nullptr;
    uint8_t _StepMask = 0;
    uint8_t _DirMask  = 0;


    // State
    MotorState_t _State;
    bool _StopActive = false; // STOPP latch; only a valid STOPP release clears it.
    MotorJob_t _Job;

    // Movement profile
  
   volatile  MotorProfile_t  _ProfileElement;  // Aktives Element 

    // Position

    uint16_t _TargetPosition;
    // Results of Movment 
    MotorJobResult_t _JobResult;
    volatile uint16_t _Position;    // Absolute position, valid after reference run
    volatile uint16_t _TotalLength = 0U; // Calibrated span measured from position 1000
    volatile bool _DirectionRight = true; // logische Bewegungsrichtung fuer ISR

   void disableAndInvalidate(bool latchStop);

    bool Prepare();
    void updateReference();
    void startReferenceMotion(bool directionRight, uint16_t frequency);
    void stopReferenceMotion();
    void finishReference(bool valid);

    volatile uint8_t* _LeftLimitInputPort = nullptr;
    volatile uint8_t* _RightLimitInputPort = nullptr;
    volatile uint8_t* _ReferenceInputPort = nullptr;
    uint8_t _LeftLimitMask = 0U;
    uint8_t _RightLimitMask = 0U;
    uint8_t _ReferenceInputMask = 0U;

    static constexpr uint16_t REFERENCE_START_POSITION = 12500U;
    static constexpr uint16_t REFERENCE_LEFT_POSITION = 1000U;
    static constexpr uint16_t REFERENCE_BACKOFF_STEPS = 1200U;
    static constexpr uint16_t REFERENCE_SHIFT_STEPS = 500U;


   // Prüfungen der Daten  
   bool prepareSetPosition();
   bool prepareLeft();
   bool prepareRight();
   bool prepareSetTrack();
   bool CheckBorder();
   bool prepareParameter();
   
   
    static MotorProfileParam_t _ProfileParam;

    // TODO: timing / step counter
    // Calculate the movement profile.
     bool calcProfile(uint16_t distance );

    // ----------------------------------------------------------------
    // Timer 1
    // ----------------------------------------------------------------
    void Timer1_Init();
    void Timer1_Start();
    void Timer1_Stop();
    static uint16_t current_profile_ramp_interval();

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
    volatile bool _TimerValid = false;   // Profil ist vorbereitet
    volatile bool _StepRun = false;      // STEP-Ausgabe freigegeben
    volatile bool _StepLevel = false;    // aktueller STEP-Pegel
    volatile uint16_t _ProfileRampCounter = 0;

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
    static constexpr uint16_t POS_MAX = 25000;

    // Timergrenzen
    // Kleinerer Wert = höhere Geschwindigkeit.
    // TIMER_V_MAX entspricht der oberen Frequenzgrenze,
    // TIMER_V_MIN der unteren Geschwindigkeitsgrenze.
    static constexpr uint16_t TIMER_MIN = TIMER_V_MAX;
    static constexpr uint16_t TIMER_MAX = TIMER_V_MIN;

    // Timeränderung pro Schritt
    // negativ = schneller
    // positiv = langsamer
    static constexpr int16_t ACCEL_MIN = -100;
    static constexpr int16_t ACCEL_MAX = 100;

};