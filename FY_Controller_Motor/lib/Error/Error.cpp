#include "Error.h"

// -----------------------------------------------------------------------------
// Error
// -----------------------------------------------------------------------------
//
// Die zentrale Fehlerverwaltung speichert bis zu drei aktuell aktive Fehler.
// Die Module erkennen und melden ihre Fehler selbst.

// -----------------------------------------------------------------------------

void Error::begin()
{
    for (uint8_t i = 0; i < MAX_ERRORS; ++i)
        _errors[i] = FY_ERROR_NONE;
}

void Error::update()
{
    // Fehlererkennung bleibt bei den jeweiligen Modulen.
}

void Error::setError(uint8_t errorCode)
{
    if (errorCode == FY_ERROR_NONE)
        return;

    const uint8_t location =
        static_cast<uint8_t>(errorCode & FY_ERROR_LOCATION_MASK);

    if (location == 0U)
        return;

    // Pro Fehlerort wird nur ein aktuell gültiger Fehler gehalten.
    // Ein neuer Fehler desselben Moduls ersetzt den bisherigen Eintrag.
    for (uint8_t i = 0; i < MAX_ERRORS; ++i)
    {
        if ((_errors[i] & FY_ERROR_LOCATION_MASK) == location)
        {
            _errors[i] = errorCode;
            return;
        }
    }

    // Neuen Fehler in den ersten freien Platz eintragen.
    for (uint8_t i = 0; i < MAX_ERRORS; ++i)
    {
        if (_errors[i] == FY_ERROR_NONE)
        {
            _errors[i] = errorCode;
            return;
        }
    }

    // Die ersten drei aktuellen Fehler bleiben erhalten.
}

void Error::clearError(uint8_t errorLocation)
{
    const uint8_t location =
        static_cast<uint8_t>(errorLocation & FY_ERROR_LOCATION_MASK);

    if (location == 0U)
        return;

    for (uint8_t i = 0; i < MAX_ERRORS; ++i)
    {
        if ((_errors[i] & FY_ERROR_LOCATION_MASK) == location)
            _errors[i] = FY_ERROR_NONE;
    }
}

uint8_t Error::getError(uint8_t index) const
{
    if (index >= MAX_ERRORS)
        return FY_ERROR_NONE;

    return _errors[index];
}

uint8_t Error::getSystemByte() const
{
    uint8_t status = 0;

    for (uint8_t i = 0; i < MAX_ERRORS; ++i)
    {
        if (_errors[i] != FY_ERROR_NONE)
            status |= static_cast<uint8_t>(1U << i);
    }

    return status;
}

// -----------------------------------------------------------------------------
// Alte freie Fehler-API
// -----------------------------------------------------------------------------
//
// Vorläufig auskommentiert. Die bisherige Implementierung wird nicht gelöscht,
// damit bei der späteren Umstellung sichtbar bleibt, was ersetzt wurde.
//
// namespace
// {
//     uint8_t currentError = FY_ERROR_NONE;
// }
//
// void setError(uint8_t errorCode)
// {
//     currentError = errorCode;
// }
//
// uint8_t getError()
// {
//     return currentError;
// }
//
// bool hasError()
// {
//     return currentError != FY_ERROR_NONE;
// }
//
// void clearError()
// {
//     currentError = FY_ERROR_NONE;
// }
