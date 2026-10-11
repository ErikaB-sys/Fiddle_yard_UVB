#include "FY_System.h"
#include "HW_Config.h"
#include "Main.h"
#include "Protokoll.h"
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
Error          FY_Error;
UART_Context_t Main_Context;

FY_SystemStatus_t systemStatus;
FY_ModuleContext_t FY_Modules{};

// Main-owned snapshot: UART never reads the ISR-owned position counter.
int32_t motorPositionSnapshot = static_cast<int32_t>(POSITION_UNKNOWN);

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
  Main_Context.error = &FY_Error;
  Main_Context.motorPosition = &motorPositionSnapshot;
  Main_Context.Track_INFO = nullptr;
  Main_Context.motorSpeed = nullptr;

  FY_Modules = {
      &FY_motor,
      &FY_uart,
      &FY_Keyboard,
      &FY_Switches
  };

  FY_Error.begin();

  FY_uart.begin(Main_Context, FY_Modules);

  FY_motor.begin();
  motorPositionSnapshot = static_cast<int32_t>(FY_motor.getPosition());

  FY_Keyboard.begin();
  FY_Keyboard.setMode(systemStatus.mode);

  FY_Switches.begin();

#ifdef DebugSwitches
  Serial.print(F("Display adress: "));
  Serial.println(DISPLAY_ADDRESS, HEX);
  Serial.print(F("Portexpander adress: "));
  Serial.println(PORT_EXPANDER_ADDRESS, HEX);

  Serial.print(F("Refsw: "));
  Serial.print(FY_Switches.getAnalogValue(Switches::Id::REF));
  Serial.print(F("  "));
  Serial.println(FY_Switches.getDigitalValue(Switches::Id::REF), DEC);

  Serial.print(F("TrimswL: "));
  Serial.print(FY_Switches.getAnalogValue(Switches::Id::TRIM_LEFT));
  Serial.print(F("  "));
  Serial.println(FY_Switches.getDigitalValue(Switches::Id::TRIM_LEFT), DEC);

  Serial.print(F("TrimswR: "));
  Serial.print(FY_Switches.getAnalogValue(Switches::Id::TRIM_RIGHT));
  Serial.print(F("  "));
  Serial.println(FY_Switches.getDigitalValue(Switches::Id::TRIM_RIGHT), DEC);
#endif

  printFirmwareInfo();

  Serial.println(F("init done "));
}

void loop()
{
  static uint32_t lastPositionSnapshotMs = 0U;
  static uint32_t tSwitches = 0U;
  static uint32_t tButtons = 0U;
  static uint32_t tDisplay = 0U;
  static bool motorWasMoving = false;
  static bool displayDirty = true;

  const uint32_t now = millis();

  // 1. Read physical inputs on the HW-test timeslice (15 ms).
  if ((now - tSwitches) >= 15UL)
  {
    tSwitches = now;
    // TODO: Add the Switches input-image update here.
    // FY_Switches.update();
  }

  // 2. Refresh the position snapshot BEFORE UART processing.
  // UART responses therefore use the latest published snapshot.
  const bool motorMoving = FY_motor.isMoving();

  if (motorMoving && ((now - lastPositionSnapshotMs) >= 100UL))
  {
    motorPositionSnapshot = static_cast<int32_t>(FY_motor.getPosition());
    lastPositionSnapshotMs = now;
  }
  else if (!motorMoving && motorWasMoving)
  {
    // Capture the final position immediately after movement ends.
    motorPositionSnapshot = static_cast<int32_t>(FY_motor.getPosition());
    lastPositionSnapshotMs = now;
  }

  motorWasMoving = motorMoving;

  // 3. Communication runs on every loop pass.
  // update() also handles any response that is due.
  FY_uart.update();

  // 4. Apply newly received commands to the motor after UART processing.
  FY_motor.Update();

  // 5. Apply the shared hardware output image here once Outputs is integrated.
  // update_outputs();

  // 6. Reserved timeslice for keyboard processing (25 ms).
  if ((now - tButtons) >= 25UL)
  {
    tButtons = now;
    // FY_Keyboard.Update();
  }

  // 7. Reserved timeslice for display refresh (200 ms or dirty flag).
  if (((now - tDisplay) >= 200UL) || displayDirty)
  {
    tDisplay = now;
    // FY_display.update();
    displayDirty = false;
  }
}
