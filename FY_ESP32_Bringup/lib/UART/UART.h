#pragma once
#include "FY_Bringup.h"
namespace fy {
// Huginn owns UART, BabelFish and UART task lifecycle.
BaseType_t Huginn_startUART(BringupContext* context);
}
