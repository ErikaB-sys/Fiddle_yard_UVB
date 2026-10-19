#pragma once
#include "FY_Bringup.h"
namespace fy {
// Muninn owns motor mock task and test LED; no real motor outputs.
void Muninn_initMotorMock();
BaseType_t Muninn_startMotorMock(BringupContext* context);
}
