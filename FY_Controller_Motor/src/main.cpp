
#include "FY_System.h"
#include "Config.h"
//#include "Main.h"
#include "UART.h"


// Variables for the project
//FY_System is a variable of type FY_SystemStatus that represents the current status of the system. It is defined to hold system-related information, allowing for monitoring and management of system operations.
FY_SystemInitStatus_t FY_System; // System status variables

// FY instances of the classes for the project
// Display       FY_display(DISPLAY_ADDRESS); // Create a Display object with the specified I2C address
// PortExpander  FY_portexpander(PORT_EXPANDER_ADDRESS); // Create a PortExpander object with the specified I2C address
// Buttons       FY_buttons(&FY_portexpander); // Create a Buttons object and pass the PortExpander object to it for button management
//Switches       FY_Switches; // Create a Switches object for managing switches
UART             FY_uart;  // Create a UART object with the specified device and baud rate
UART_Context_t   Main_Context;
//Motor            FY_motor();

FY_SystemStatus_t systemStatus;

void setup() {
// first of all setup data structurs 
UART_Context_t uartContext
{
    &systemStatus,
   // &uartError,
  //  &motorPosition,
  //  &motorTrack,
  //  &motorSpeed
};


  // Initialize serial communication
  FY_uart.begin(Main_Context);
  // UART error abfragen !


/*
  // init I2C Display (small OLED 128x64)
  if (FY_display.begin()) {
    FY_System.setDisplayStatus(true);
    FY_display.INITScreen();
  } else {
    FY_System.setDisplayStatus(false);
  }

  // init I2C Portexpander (MCP23017)
  FY_portexpander.begin();
  if (FY_portexpander.isConnected()) {
    FY_System.setPortExpanderStatus(true);
  } else {
    FY_System.setPortExpanderStatus(false);
  }

  // If no Uart and no Display / Portexpander is connected, the system will not work properly.
  if (!FY_System.getUartStatus() && !FY_System.getDisplayStatus() && !FY_System.getExpanderStatus()) {
    FY_System.setSystemError(true);
  } else {
    FY_System.setSystemError(false);
  }

  // and stop here, because the system will not work properly without Uart and Display / Portexpander
  if (FY_System.systemError) {
    while (true) {
      delay(400);
    }
  }

  // init buttons and LED
  FY_buttons.begin();
  // Init switches and controls
  FY_Switches.beginn();
  // init Motor driver
  FY_motor.begin();
*/
  #ifdef DebugSwitches
  // send init result on Uart
  Serial.print(F("Display adress: "));
  Serial.println(FY_display.getAddress(), HEX);
  Serial.print(F("Portexpander adress: "));
  Serial.println(FY_portexpander.getAddress(), HEX);

  Serial.print(F("Refsw: "));
  Serial.print(FY_Switches.getAnalogValue(SWITCH_REF));
  Serial.print(F("  "));
  Serial.println(FY_Switches.GetDigitalValue(SWITCH_REF), DEC);

  Serial.print(F("TrimswL: "));
  Serial.print(FY_Switches.getAnalogValue(SWITCH_TRIM_LEFT));
  Serial.print(F("  "));
  Serial.println(FY_Switches.GetDigitalValue(SWITCH_TRIM_LEFT), DEC);

  Serial.print(F("TrimswR: "));
  Serial.print(FY_Switches.getAnalogValue(SWITCH_TRIM_RIGHT));
  Serial.print(F("  "));
  Serial.println(FY_Switches.GetDigitalValue(SWITCH_TRIM_RIGHT), DEC);
#endif


Serial.println(F("init done "));
/*
  // Clear screen
  FY_display.clearDisplay();
  FY_display.display();

  */
}

void loop() {
  // put your main code here, to run repeatedly:
  //get all informatins 
  FY_uart.update();
  
  /*
  FY_buttons.Update();
  FY_Switches.update();
  // Zentral state mashine for the project
  FY_motor.update();
  //Tell the world what is going on 
  FY_display.update();
*/
  }


