#include "FY_System.h"
#include "Config.h"
#include "Main.h"
#include "Motor.h"
#include "UART.h"
#include "Error.h"
#include "Keyboard.h"
#include "Switches.h"
#include "Firmware.h"

FY_SystemInitStatus_t FY_System;

Switches       FY_Switches;
UART           FY_uart;
Motor          FY_motor;
Keyboard       FY_Keyboard;
UART_Context_t Main_Context;

FY_SystemStatus_t systemStatus;
FY_ModuleContext_t FY_Modules{};

void printFirmwareInfo()
{
  Serial.print(F("FW: "));
  Serial.println(FW_NAME);

  Serial.print(F("VER: "));
  Serial.println(FW_VERSION);

  Serial.print(F("BUILD: "));
  Serial.print(FW_BUILD);
  Serial.print(F(" "));
  Serial.println(FW_TIME);

  Serial.print(F("GIT: "));
  Serial.println(FW_GIT_COMMIT);
}

void setup()
{
  Main_Context.systemStatus = &systemStatus;
  Main_Context.uartError = nullptr;
  Main_Context.motorPosition = nullptr;
  Main_Context.Track_INFO = nullptr;
  Main_Context.motorSpeed = nullptr;

  FY_Modules = {
      &FY_motor,
      &FY_uart,
      &FY_Keyboard,
      &FY_Switches
  };

  FY_uart.begin(Main_Context, FY_Modules);

  FY_motor.begin(
      MOTOR_DIR_PIN,
      MOTOR_PWM_PIN,
      MOTOR_ENABLE_PIN
  );

  FY_Keyboard.begin(
      BUTTON_LEFT,
      BUTTON_RIGHT,
      BUTTON_GO,
      BUTTON_STOP,
      DISPLAY_ADDRESS,
      PORT_EXPANDER_ADDRESS
  );

  FY_Switches.begin(
      SWITCH_REF_D,
      SWITCH_TRIM_LEFT_D,
      SWITCH_TRIM_RIGHT_D,
      SWITCH_TIMING_BELT_D
  );

#ifdef DebugSwitches
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

  printFirmwareInfo();

  Serial.println(F("init done "));
}

void loop()
{
  FY_uart.update();

  /*
  FY_Keyboard.Update();
  FY_Switches.update();
  FY_motor.Update();
  FY_display.update();
  */
}
