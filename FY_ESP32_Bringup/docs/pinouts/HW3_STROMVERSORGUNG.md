# HW3 – ESP32-Stromversorgung, USB und DC/DC (Arbeitsentscheidung)

**Quellen:** [AZ-Delivery PDF-Verzeichnis](../../../Documents/Datasheets/ESP32_AZ_Delivery_38Pin_REFERENZEN.md), [#161 Trägerplatine](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/161), [#155 Boot-Sicherheit](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/155). **Stand 21.10.2026: Entwurf / noch keine elektrisch geprüfte HW3-Schaltung.**

## Was der Hersteller-Schaltplan zeigt

Beim AZ-Delivery ESP32 NodeMCU 38-Pin / CP2102 liegt der **3V3-Header direkt am Ausgang des onboard AMS1117-3.3-Reglers**. Dieser versorgt sich aus dem 5-V-Pfad (USB-Einspeisung über Diode). **Keine nachgewiesene Entkopplung zwischen dem Header und dem LDO-Ausgang.** Prüfen, ob unsere physische Platinenrevision tatsächlich diesem Schaltplan entspricht.

## Warnung für unsere Lochraster-HW3

Geplant: separater DC/DC-Wandler (12 V auf 3,3 V), ESP32, A4988/STEP-DIR-ENABLE, I²C-/SPI-Stecker.

**Nicht so verdrahten:** DC/DC-Ausgang fest auf `ESP32.3V3`, gleichzeitig ESP32 per USB versorgen. Zwei Versorgungsquellen treiben dieselbe Schiene. Selbst bei `12 V AUS` kann die USB-versorgte 3V3-Schiene den **abgeschalteten** externen DC/DC-Ausgang rückspeisen. Ob dieser Wandler Rückstrom toleriert, ist unbekannt.

### Konservative HW3-Lösung zur Verdrahtung

1. **Steckbrücke/Schalter für externe ESP32-3V3-Einspeisung vorsehen** – nicht einfach dauerhaft durchverbinden.
2. **FLASH/USB:** 12-V-Motorversorgung AUS **und** externe 3V3-Einspeisung zum ESP32 **physisch abgetrennt**. Nur USB versorgt den ESP32. Prüfen, ob andere GPIO-/I²C-Geräte durch die USB-Versorgung ungewollt rückgespeist werden.
3. **BETRIEB:** USB-5V/PC-Verbindung getrennt; externe Versorgung erst dann einschalten. Nur eine Quelle speist das Board.
4. **VMOT für A4988 unabhängig abschalten**; dessen Kondensator nach Abschaltung entladen lassen. Ungewollte GPIO-Pegel während Reset/Bootloader bleiben ohne Hardware-Interlock grundsätzlich möglich.
5. **Wenn Debuggen mit angeschlossenem USB und externer Versorgung unbedingt gleichzeitig benötigt wird:** zuerst definierte USB-VBUS-Trennung oder eine dafür ausgelegte Power-Mux-/Ideal-Diode-Lösung mit realem Schaltplan und Messung entwickeln; **keine Annahme**, ein gewöhnliches USB-Kabel oder die Borddiode würde das automatisch absichern.

Die Lösung mit separat gestecktem 3V3-Jumper ist für HW3 einfach nachvollziehbar und verhindert, dass wir eine vermeidbare Rückspeisung in die Lochrasterkarte konstruieren.

## Vor dem ersten Einschalten prüfen

- [ ] Tatsächlich verbauter ESP32-Regler / Boardrevision mit AZ-Schaltplan verglichen.
- [ ] DC/DC-Datenblatt: Rückstrom vom Ausgang bei Eingang AUS geprüft.
- [ ] 3V3-Jumper oder Versorgungsauswahl mechanisch/elektrisch eindeutig.
- [ ] USB alleine, externe 12 V AUS: kein ungewollter Strom auf externem 3,3-V-Wandlerausgang.
- [ ] Externe Versorgung alleine, USB getrennt: gemessene 3,3 V am Board.
- [ ] A4988-VMOT unabhängig unterbrochen und POR-Enable/Sleep-Zustände gemessen.

**Keine Freigabe durch Dokumentation**: Die Liste ist ein Aufbau- und Prüfplan.
