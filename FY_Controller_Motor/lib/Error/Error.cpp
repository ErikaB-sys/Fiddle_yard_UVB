#include <Arduino.h>
#include "Error.h"

namespace
{
    uint8_t currentError = FY_ERROR_NONE;
}

void setError(uint8_t errorCode)
{
    currentError = errorCode;
}

uint8_t getError()
{
    return currentError;
}

bool hasError()
{
    return currentError != FY_ERROR_NONE;
}

void clearError()
{
    currentError = FY_ERROR_NONE;
}
