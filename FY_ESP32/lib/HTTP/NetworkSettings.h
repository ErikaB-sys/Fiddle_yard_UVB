#pragma once

/** @file NetworkSettings.h
 *  @brief Separater NVS-Speicher fuer WLAN-Konfiguration; niemals im Motor-NVM.
 */
#include <Arduino.h>

namespace fy {
struct NetworkSettings {
    String ssid;
    String password;
};
bool NetworkSettings_valid(const NetworkSettings& settings);
bool NetworkSettings_load(NetworkSettings& settings);
bool NetworkSettings_save(const NetworkSettings& settings);
} // namespace fy
