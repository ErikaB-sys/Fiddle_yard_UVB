#include "NVM.h"

#include <Preferences.h>
#include <string.h>

namespace fy {
namespace {

constexpr size_t kHeaderSize = 8U;
constexpr size_t kCrcSize = 4U;
constexpr size_t kMaxPayload = 256U;
constexpr size_t kMaxRecordSize = kHeaderSize + kMaxPayload + kCrcSize;
constexpr uint8_t kMagic[4]{'F', 'Y', 'N', 'V'};

constexpr char kNamespace[] = "fy_uvb";

struct NvmDescriptor_t {
    NvmRecord id;
    const char* key;
    uint16_t version;
    uint16_t length;
};

// Append-only: IDs, keys, Feldreihenfolge und Version nicht ohne Migration aendern.
constexpr NvmDescriptor_t kDescriptors[]{
    {NvmRecord::CALIBRATION,     "calib",  1U, 12U},
    {NvmRecord::TRACK_POSITIONS, "tracks", 1U, 20U},
    {NvmRecord::MOTOR_PARAMETERS, "motor", 2U, 34U}
};
static_assert(sizeof(kDescriptors) / sizeof(kDescriptors[0]) == 3U,
              "NVM Segmenttabelle pruefen");

const NvmDescriptor_t* descriptor(NvmRecord id) {
    for (const auto& entry : kDescriptors) {
        if (entry.id == id) return &entry;
    }
    return nullptr;
}

uint16_t read16(const uint8_t* p) {
    return static_cast<uint16_t>(static_cast<uint16_t>(p[0]) |
                                 (static_cast<uint16_t>(p[1]) << 8U));
}

uint32_t read32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8U) |
           (static_cast<uint32_t>(p[2]) << 16U) |
           (static_cast<uint32_t>(p[3]) << 24U);
}

void write16(uint8_t* p, uint16_t value) {
    p[0] = static_cast<uint8_t>(value);
    p[1] = static_cast<uint8_t>(value >> 8U);
}

void write32(uint8_t* p, uint32_t value) {
    p[0] = static_cast<uint8_t>(value);
    p[1] = static_cast<uint8_t>(value >> 8U);
    p[2] = static_cast<uint8_t>(value >> 16U);
    p[3] = static_cast<uint8_t>(value >> 24U);
}

// CRC-32/ISO-HDLC: reflektiert, Polynom 0xEDB88320, init/final XOR FFFFFFFF.
uint32_t crc32(const uint8_t* bytes, size_t length) {
    uint32_t crc = UINT32_MAX;
    for (size_t i = 0U; i < length; ++i) {
        crc ^= bytes[i];
        for (uint8_t bit = 0U; bit < 8U; ++bit)
            crc = (crc >> 1U) ^ ((crc & 1U) != 0U ? 0xEDB88320UL : 0U);
    }
    return ~crc;
}

bool validLayout(const uint8_t* record, size_t size) {
    if (size < kHeaderSize + kCrcSize || size > kMaxRecordSize ||
        memcmp(record, kMagic, sizeof(kMagic)) != 0) return false;
    const uint16_t payloadLength = read16(record + 6U);
    return payloadLength > 0U && payloadLength <= kMaxPayload &&
           size == kHeaderSize + payloadLength + kCrcSize;
}

size_t makeRecord(uint8_t* record, uint16_t version,
                  const void* payload, size_t length) {
    memcpy(record, kMagic, sizeof(kMagic));
    write16(record + 4U, version);
    write16(record + 6U, static_cast<uint16_t>(length));
    memcpy(record + kHeaderSize, payload, length);
    const size_t beforeCrc = kHeaderSize + length;
    write32(record + beforeCrc, crc32(record, beforeCrc));
    return beforeCrc + kCrcSize;
}

bool acceptableCrc(const uint8_t* record, size_t size, bool allowInvalidated) {
    const uint32_t expected = crc32(record, size - kCrcSize);
    const uint32_t stored = read32(record + size - kCrcSize);
    return stored == expected || (allowInvalidated && stored == ~expected);
}

} // anonymous namespace

NvmResult NVM::load(NvmRecord id, void* data) const {
    const NvmDescriptor_t* desc = descriptor(id);
    if (desc == nullptr || data == nullptr) return NvmResult::INVALID_ARG;
    const char* key = desc->key;
    const uint16_t version = desc->version;
    const size_t length = desc->length;

    Preferences prefs;
    // Beim ersten Boot existiert der Namespace noch nicht. Schreibend
    // oeffnen erzeugt ihn; ein fehlender Key wird dann als NOT_FOUND erkannt.
    // Sonst wuerde ein jungfraeuliches NVS faelschlich IO_ERROR liefern.
    if (!prefs.begin(kNamespace, false)) return NvmResult::IO_ERROR;
    if (!prefs.isKey(key)) {
        prefs.end();
        return NvmResult::NOT_FOUND;
    }
    const size_t storedSize = prefs.getBytesLength(key);
    if (storedSize < kHeaderSize + kCrcSize || storedSize > kMaxRecordSize) {
        prefs.end();
        return NvmResult::INVALID_DATA;
    }
    uint8_t record[kMaxRecordSize]{};
    const size_t readSize = prefs.getBytes(key, record, storedSize);
    prefs.end();
    if (readSize != storedSize) return NvmResult::IO_ERROR;
    if (!validLayout(record, storedSize) || read16(record + 4U) != version ||
        read16(record + 6U) != length ||
        !acceptableCrc(record, storedSize, false)) return NvmResult::INVALID_DATA;
    memcpy(data, record + kHeaderSize, length);
    return NvmResult::OK;
}

NvmResult NVM::save(NvmRecord id, const void* data) const {
    const NvmDescriptor_t* desc = descriptor(id);
    if (desc == nullptr || data == nullptr) return NvmResult::INVALID_ARG;
    const char* key = desc->key;
    const uint16_t version = desc->version;
    const size_t length = desc->length;

    uint8_t record[kMaxRecordSize]{};
    const size_t recordSize = makeRecord(record, version, data, length);
    Preferences prefs;
    if (!prefs.begin(kNamespace, false)) return NvmResult::IO_ERROR;
    if (prefs.isKey(key) && prefs.getBytesLength(key) == recordSize) {
        uint8_t prior[kMaxRecordSize]{};
        if (prefs.getBytes(key, prior, recordSize) != recordSize) {
            prefs.end();
            return NvmResult::IO_ERROR;
        }
        if (memcmp(prior, record, recordSize) == 0) {
            prefs.end();
            return NvmResult::UNCHANGED;
        }
    }
    const size_t written = prefs.putBytes(key, record, recordSize);
    prefs.end();
    return written == recordSize ? NvmResult::OK : NvmResult::IO_ERROR;
}

NvmResult NVM::invalidate(NvmRecord id) const {
    const NvmDescriptor_t* desc = descriptor(id);
    if (desc == nullptr) return NvmResult::INVALID_ARG;
    const char* key = desc->key;
    Preferences prefs;
    if (!prefs.begin(kNamespace, false)) return NvmResult::IO_ERROR;
    if (!prefs.isKey(key)) {
        prefs.end();
        return NvmResult::NOT_FOUND;
    }
    const size_t size = prefs.getBytesLength(key);
    if (size < kHeaderSize + kCrcSize || size > kMaxRecordSize) {
        prefs.end();
        return NvmResult::INVALID_DATA;
    }
    uint8_t record[kMaxRecordSize]{};
    if (prefs.getBytes(key, record, size) != size) {
        prefs.end();
        return NvmResult::IO_ERROR;
    }
    if (!validLayout(record, size) || read16(record + 4U) != desc->version ||
        read16(record + 6U) != desc->length ||
        !acceptableCrc(record, size, true)) {
        prefs.end();
        return NvmResult::INVALID_DATA;
    }
    const uint32_t invalidCrc = ~crc32(record, size - kCrcSize);
    if (read32(record + size - kCrcSize) == invalidCrc) {
        prefs.end();
        return NvmResult::UNCHANGED;
    }
    write32(record + size - kCrcSize, invalidCrc);
    const size_t written = prefs.putBytes(key, record, size);
    prefs.end();
    return written == size ? NvmResult::OK : NvmResult::IO_ERROR;
}

} // namespace fy
