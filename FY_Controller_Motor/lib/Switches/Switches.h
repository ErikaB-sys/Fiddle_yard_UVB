#pragma once
// Switches.h
// This file contains the definitions and functions related to the switches used in the project.
// It defines the switch types, their analog and digital values, and provides functions to initialize and read the switches.
// defines the Class Switches, which is used to manage the switches and controls in the project. 
// It provides methods to initialize the switches, read their values, and update their states. 
// The class also includes methods to get the analog and digital values of the switches, as well as to check if a switch is pressed or released.
#define Max_Switches 4

class Switches {
public:
    struct Switch {
        uint8_t analogPin;
        uint8_t digitalPin;
        int switchId;
        int analogValue;
        bool digitalValue;
    };

    Switch switches[Max_Switches]; // Array to hold the switches

    Switches();
    
    void begin(uint8_t analogPin, uint8_t digitalPin, int switchId);
    void update();   // Update the state of all switches
    bool isReleased(int switchId); // Check if a switch is released
    bool isPressed(int switchId);  // Check if a switch is pressed
    int  getAnalogValue(int switchId); // get analog value  from Structur  
    bool getDigitalValue(int switchId);//  get digital Value from Structur 
    
private:

    void initializeSwitch(int switchId); // init
    int  readAnalogValue(int switchId); //
    bool readDigitalValue(int switchId); //




};