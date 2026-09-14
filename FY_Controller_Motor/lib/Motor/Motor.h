#pragma once
// Motor.h
// Declarations for motor control used by the project.

enum MotorState_t
{
    IDLE,       // Motor is idle.
    CONF,       // Change of motor & moving parameter
    MOVING,     // Motor is moving.
    STOPPED,    // Motor has been stopped.
    ERROR       // Motor is in an error state.
};

enum MotorProfile_t
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
    uint8_t Accel;  // Acceleration for this profile element.
};




class Motor
{
public:
    Motor();


    // Update the motor control state.
    void Update();

    // Initialize the motor control pins.
    bool begin(uint8_t Dir_pin, uint8_t Step_pin, uint8_t ENA_pin);

    // Start a move to the target position. Requires the motor to be referenced.
    void move2Pos(long targetPosition);

    // Move the specified number of steps. Referencing is not required.
    void moveSteps(long Steps);

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

    // State
    MotorState_t _State;

    // Movement profile
    Profilelement_t Motor_profil [6];
    uint8_t _ProfileElement;
    uint16_t _StepsRemaining;

    // Timer
    uint16_t _TimerValue;
    /*
    MotorProfile_t _Profile;
    uint8_t  _ProfileElement;
    */

    // TODO: timing / step counter
    // Calculate the movement profile.
    bool calcprofil( );


};


  