#include "NetworkSettings.h"
#include <Preferences.h>
#include <cstring>

namespace fy {
namespace {
// Eine atomar gespeicherte Einheit verhindert SSID-/Passwort-Mischstaende
// nach Stromausfall. Eigener Namespace, FY-Kalibrierung bleibt unangetastet.
constexpr char kNamespace[] = "fy_wifi";
constexpr char kKey[] = "credentials";
constexpr uint32_t kMagic = 0x314E5746UL;  // "FWN1"
struct StoredNetwork {
    uint32_t magic;
    char ssid[33];
    char password[64];
    uint32_t crc;
};
uint32_t crc32(const uint8_t* bytes, size_t length) {
    uint32_t value = UINT32_MAX;
    for (size_t i = 0; i < length; ++i) {
        value ^= bytes[i];
        for (uint8_t bit = 0; bit < 8U; ++bit)
            value = (value >> 1U) ^ ((value & 1U) ? 0xEDB88320UL : 0U);
    }
    return ~value;
}
uint32_t recordCrc(const StoredNetwork& data) {
    return crc32(reinterpret_cast<const uint8_t*>(&data),
                 offsetof(StoredNetwork, crc));
}
} // namespace

bool NetworkSettings_valid(const NetworkSettings& settings) {
    return settings.ssid.length() >= 1U &&
           settings.ssid.length() <= 32U &&
           settings.password.length() >= 8U &&
           settings.password.length() <= 63U;
}

bool NetworkSettings_load(NetworkSettings& settings) {
    settings = {};
    Preferences pref;
    if (!pref.begin(kNamespace, true)) return false;
    StoredNetwork data{};
    const bool lengthOk = pref.getBytesLength(kKey) == sizeof(data);
    const bool readOk = lengthOk &&
                        pref.getBytes(kKey, &data, sizeof(data)) == sizeof(data);
    pref.end();
    if (!readOk || data.magic != kMagic || recordCrc(data) != data.crc ||
        data.ssid[sizeof(data.ssid)-1U] != '\0' &&
        std::memchr(data.ssid, 0, sizeof(data.ssid)) == nullptr ||
        data.password[sizeof(data.password)-1U] != '\0' &&
        std::memchr(data.password, 0, sizeof(data.password)) == nullptr)
        return false;
    NetworkSettings candidate{String(data.ssid), String(data.password)};
    if (!NetworkSettings_valid(candidate)) return false;
    settings = candidate;
    return true;
}

bool NetworkSettings_save(const NetworkSettings& settings) {
    if (!NetworkSettings_valid(settings)) return false;
    StoredNetwork data{};
    data.magic = kMagic;
    std::memcpy(data.ssid, settings.ssid.c_str(), settings.ssid.length());
    std::memcpy(data.password, settings.password.c_str(),
                settings.password.length());
    data.crc = recordCrc(data);
    Preferences pref;
    if (!pref.begin(kNamespace, false)) return false;
    const bool ok = pref.putBytes(kKey, &data, sizeof(data)) == sizeof(data);
    pref.end();
    return ok;
}
} // namespace fy
