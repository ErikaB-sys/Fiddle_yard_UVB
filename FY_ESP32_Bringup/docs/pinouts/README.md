# Hardware – zentrale Pinout-Navigation

**Issues:** [#151](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/151), [#155](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/155), [#161](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/161).

- [ESP32-WROOM / HW2 Pinout](01_PINOUT_ESP32.md)
- [Nano-ESP32 Adapter](ADAPTER_NANO_ESP32.md)
- [HW_Config.h](../../include/HW_Config.h): **tatsächlich im Code benutzte GPIO-Zahlen**.
- [Nano-Referenz](../../reference/02_PINOUT_NANO.md)
- [Keyboard-HW-Test-Pinout](../../../FY_HW_TEST/FY_HW_TEST/include/HW_Test.h) (Branch-Verfügbarkeit prüfen).
- [Datenblätter/extern](../../../Documents/Datasheets/)
- [AZ-Delivery Originalquellen (ESP32 V2 / CP2102 / 38 Pin)](../../../Documents/Datasheets/ESP32_AZ_Delivery_38Pin_REFERENZEN.md)
- [HW3: externe 3,3 V und USB – Versorgungsumschaltung](HW3_STROMVERSORGUNG.md)

**Eine Pinout-Tabelle, mehrere Links.** Platinenverdrahtung, Pegel, STEP/DIR/ENABLE und A4988-Microstep-Modus vor Aussagen über „getestet“ gegen Firmware und Messung prüfen. **VMOT beim Flashen/Reset trennen**, Hardware-Failsafe nach #155/#161 offen. Status-/Inkonsistenzliste: [VERIFICATION_GAPS.md](../VERIFICATION_GAPS.md).
