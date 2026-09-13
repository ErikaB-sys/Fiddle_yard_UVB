#pragma once
// Motor.h
// Declarations for motor control used by the project.

enum MotorState
{
    IDLE,       // Motor is idle.
    MOVING,     // Motor is moving.
    STOPPED,    // Motor has been stopped.
    ERROR       // Motor is in an error state.
};

enum Motor_profil
{
    ACC1,      // Start acceleration.
    ACC2,      // Increase speed.
    KONST,     // Move at constant speed.
    BRE1,      // Brake from constant speed.
    BRE2,      // Brake to positioning speed.
    POSI       // Find the position at very low speed.

};

struct Profilelement
{
    uint16_t Steps; // Number of steps in this profile element.
    uint8_t Accel;  // Acceleration for this profile element.
};

class Motor
{
public:
    Motor();
    ~Motor();

    // Update the motor control state.
    void Update();

    // Initialize the motor control pins.
    bool begin(uint8_t Dir_pin, uint8_t Plus_pin, uint8_t ENA_pin);

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
    long motor_getPosition();
    long motor_getTargetPosition();
    MotorState motor_getState();

private:
    // Calculate the movement profile.
    bool calcprofil( );


};

 ISR (TIMER2_OVF_vect);
  