# FY ESP32 – UART port, phase 2 (Nano protocol)

Branch: `feature/FY_ESP32_Port`. Scope: issue #156.

## Ported now

- Nano `Protokoll.h` command/response IDs and CommandType table remain authoritative.
- BabelFish retains ASCII command parsing and character echo.
- Huginn on core 0 validates ID, length and CRC-8, dispatches IMMEDIATE/EXECUTE/PRIORITY.
- `setResponse()` and `sendResponse()` emit Nano-format binary `ID | DATA | CRC8`, without length prefix, escaping or trailing newline.
- For firmware/help, `DATA` is the ASCII text (ESP32 flash mapped to address space; no AVR PROGMEM calls).
- The original Nano Error module is reused for `GET_ERROR`.
- Core 1 receives normal motor commands in `commands` queue and STOP in the independent `priorityCommands` queue. Events include sequence ID, command ID, result ID, and one-byte result data.
- The current motor backend is **still a mock**. Driver is kept disabled. No movement or reference run is simulated as successful.

## Pre-motor integration response policy

- `GET_STATUS 10`: system data `04 00` = UART initialized, state INIT.
- `GET_STATUS 20`: motor status three zero bytes (not initialized, inactive).
- `GET_STATUS 40`: switches placeholder zero; sensors not wired.
- `GET_STATUS 50`: keyboard disconnected (zero).
- `GET_ERROR`: real saved UART errors (initially four zero bytes).
- `GET_POSITION`: 32-bit little-endian `0x00007FFF`, Nano POSITION_UNKNOWN.
- `GET_TRACK 00`: Nano-style `DEV_RESPONSE 11 00` because a trustworthy track snapshot is not wired yet.
- `HELP`, `GET_FIRMWARE`: string responses with CRC.
- `GO 55 AA`: safe-disable action, ACK `07`; `GO AA 55`: motor not initialized -> NACK motor/NOT_READY.
- `STOP 55 AA` and `STOP AA 55`: priority mailbox, driver remains disabled, ACK `01` when handled by Muninn. **This is a transport/mock test, not a verified stop of an active ISR.**
- `REFERENCE`, `LEFT`, `RIGHT`, `SET_POSITION`, `SET_TRACK`, `SET_PARAM`: valid telegrams dispatched to Muninn but NACK motor/NOT_READY until the real state machine is integrated.
- `SET_MODE`: currently NACK UART/NOT_READY because Keyboard/motor dependencies are absent.
- Invalid lengths/data return Nano-format NACK with location in high nibble and reason in low nibble.
- The STOP mailbox is independent of the ordinary job queue; it is checked first. A filled STOP mailbox can still reject further STOP telegrams; real emergency-stop handling and ISR cancellation are part of the motor HAL work.

## Expected raw binary test frames

Serial terminal: 115200, send line end (CR or LF). **Responses are binary**, not printable `FY_TEST:` text. Use a hex-capable serial terminal or a small byte decoder to read them. BabelFish still echoes the input line and logs `CMD=...`.

| ASCII input | Expected response bytes (ID, DATA, CRC) |
|---|---|
| `GET_STATUS 10` | `10 04 00 03` |
| `GET_STATUS 20` | `50 00 00 00 24` |
| `GET_STATUS 40` | `41 00 C0` |
| `GET_STATUS 50` | `51 00 B0` |
| `GET_ERROR` | `F0 00 00 00 00 03` if no errors have been recorded |
| `GET_POSITION` | `20 FF 7F 00 00 3F` |
| `GET_TRACK 00` | `FF 11 00 95` |
| `GO 55 AA` | `60 07 32` |
| `GO AA 55` | `70 22 B9` |
| `STOP 55 AA` | `60 01 20` |
| `SET_TRACK 01` | `70 22 B9` |
| `GET_STATUS FF` | `70 14 3B` |

Firmware and HELP return `28 | ASCII | CRC` and `18 | ASCII | CRC`, respectively, where the text includes the build timestamp and/or help URL.

`GET_ERROR` may differ after deliberately sending invalid commands. To verify the initial zero-value response, send it immediately after reset.

## Build and hardware checks still required

1. PlatformIO build `FY_ESP32_Bringup`.
2. Flash to HW2 with motor **supply disconnected**. Confirm GPIO27 (A4988 EN) remains HIGH and GPIO25 (STEP) LOW after boot test.
3. Exercise all IMMEDIATE cases including invalid selectors and bad lengths, observe binary CRC.
4. Exercise priority STOP while normal command traffic exists and check every response.
5. Verify both UART and Muninn still run on their assigned cores; check event queue capacity and worst-case latency.
6. Only after verified timer/ISR and safety work: connect motor power and test movement.

**Neither the compile nor hardware execution of this phase has yet been verified.**
