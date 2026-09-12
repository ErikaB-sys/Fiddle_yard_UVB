#pragma once


// Protokoll.h
// This file contains the definitions for the communication protocol used in the project.
// It defines the structure of the messages exchanged between the controller and other components.
// Only for Uart communication, not for I2C or SPI.

//define the commands and responses for the communication protocol

//received commands from Uart are stored in a struct and processed by the main loop
#define MAX_COMMAND_LENGTH  8         // Maximum length of a command received from Uart in Bytes
#define CMD_GET_STATUS      0x19        // Command to get the status of the motor
#define CMD_GET_ERROR       0x1A        // Command to get the current error of the FY-Controller

#define CMD_REFERENCE       0x16        // Command to reference the motor position
#define CMD_SET_SPEED       0x2B        // Command to set the speed of the motor
#define CMD_STOPP           0x01        // Command to stop the motor
#define CMD_GO              0x07        // Command to start the motor   
#define CMD_LEFT            0x0B        // Command to move the motor left
#define CMD_RIGHT           0x0D        // Command to move the motor right

#define CMD_SET_POSITION    0x13        // Command to set the target position of the motor
#define CMD_GET_POSITION    0x15        // Command to get the current position of the motor

#define CMD_SET_TRACK       0x1C        // Command to set the track number of the motor
#define CMD_GET_TRACK       0x23        // Command to get the current track number of the motor
#define CMD_HELP            0x25        // Command to get the list of available commands

#define CMD_SET_LOCAL       0x26        // Command to set the controller to local mode
#define CMD_SET_REMOTE      0x2A        // Command to set the controller to remote mode



// Send Status to Uart
#define MAX_RESPONSE_LENGTH         8          // Maximum length of a response sent to Uart in Bytes
//general Response Codes
#define STATUS_Alive                0x03        // Status indicating that the controller is alive Cyclically send this status to Uart every ???ms
//resonse  for CMD_GET_ERROR 
#define STATUS_UnknownStatus        0xF0        // Status indicating that an unknown status has been sent to Uart
#define STATUS_UnknownCommand       0xF3        // Status indicating that an unknown command was received from Uart
#define STATUS_Alert_border         0xF5         // Status indicating that the border is unexpected reached.
#define STATUS_Reference_Error      0xF6        // Status indicating that an reference error has been occurred in the controller.
#define STATUS_Error                0xF9        // Status indicating that an error has occurred in the controller will also sent  
                                                // if  GET_Status is called and the controller is in error state. 
#define STATUS_UnknownError         0xFF        // Status indicating that an unknown error has occurred in the controller

//response for CMD_GET_STATUS
#define STATUS_INIT                 0x00        // Status indicating that the controller is initializing
#define STATUS_System_Idle          0x01        // Status indicating that the system is idle
#define STATUS_MotorMoving          0x02        // Status indicating that the motor is moving
#define STATUS_MotorStopped         0x04        // Status indicating that the motor is stopped


//response for CMD_GET_POSITION in case refrence has not been done yet the response will be STATUS_ReferenceNotDONE
#define STATUS_PositionReached      0x05        // Status indicating that the target position has been reached
#define STATUS_PositionNotReached   0x06        // Status indicating that the target position has not been reached
#define STATUS_Position             0x07        // Status indicating the current position
//response for CMD_REFERENCE
#define STATUS_ReferenceDONE        0x09        // Status indicating that the reference has be done successfully
#define STATUS_ReferenceNotDONE     0x0A        // Status indicating that the reference has not been done yet or not successfully
//response for CMD_GET_TRACK /CMD_SET_TRACK
#define STATUS_TrackSet             0x10        // Status indicating that the track number has been set
#define STATUS_TrackNotSet          0x11        // Status indicating that the track number has not been set
#define STATUS_TrackNumber          0x17        // Status indicating the current track number 
#define STATUS_TrackNotReached      0x1B        // Status indicating that the track number has not been reached yet

//response for CMD_SET_LOCAL / CMD_SET_REMOTE
#define STATUS_LocalMode            0x12        // Status indicating that the controller is in local mode
#define STATUS_RemoteMode           0x14        // Status indicating that the controller is in remote mode
//response for CMD_HELP
#define STATUS_Help                 0x18        // Status indicating that the list of available commands has been sent to Uart

