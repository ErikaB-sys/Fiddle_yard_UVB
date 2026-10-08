#include "Error.h"

// -----------------------------------------------------------------------------
// Error
// -----------------------------------------------------------------------------
//
// Der Rahmen steht.
// Die eigentliche Fehlerverwaltung wird erst im nächsten Schritt definiert.
//
// -----------------------------------------------------------------------------

void Error::begin()
{
    // TODO: zentrale Fehlerverwaltung initialisieren
}

void Error::update()
{
    // TODO: zentrale Fehlerverwaltung aktualisieren
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
