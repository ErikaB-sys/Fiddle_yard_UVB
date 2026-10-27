# UART-Portierung – Schritt 1 (19.10.2026)

**Status: eingecheckter Entwurf, noch kein ESP32-Build/Test dieses Stands.**

## Unverändert aus Nano-Baseline importiert
- `include/Protokoll.h` – Original-Befehle, Längen, Metadaten und Response-Codes.
- `lib/BabelFish/BabelFish.h/.cpp` – Original-ASCII-Adapter, Echo, Parser und CRC.
- `reference/HW_Config_Nano.h` – **nur Referenz**, wegen AVR-Portbezeichnungen nicht direkt unter ESP32 eingebunden.

Eine eigenständige `CFG.h` wurde in den geprüften Nano-Pfaden nicht gefunden; bitte tatsächlichen Dateinamen klären, bevor eine neue Konfiguration erfunden wird.

## Testkette
1. UART-Task/Core 0: `BabelFish::process` verarbeitet Terminal-Eingaben, validiert ID/Länge/CRC gegen die **unveränderte** `commandDefinitions`-Tabelle.
2. Typisierte Queue übergibt den validierten Befehl an den Dummy-Motor auf Core 1.
3. Dummy-Motor schaltet nur die eingebaute LED um und stellt ein Ereignis in die Antwort-Queue.
4. UART-Task meldet das Ereignis derzeit als `FY_TEST:`-Diagnosetext.

**Nicht als Produktivprotokoll freigegeben:** Die Antworten verwenden derzeit noch **nicht** die vollständige Nano-UART-Implementierung mit binären FY-Response-Telegrammen und ACK/NACK. Das wird als nächster Portierungsschritt durch Anpassen der bestehenden Handler und deren Datenquelle auf Motor-Snapshots/Events erfolgen. Es wurde **kein neues FY-Protokoll definiert**. Da die Nano-Protokolltabelle und der ASCII-BabelFish nicht für alle Kommandos dieselbe DATA-Länge annehmen, werden unpassende Befehle ebenso wie beim Nano-Validator abgewiesen; diese Altbestandsabweichung muss vor Funktionsparität geprüft werden.

## Probelauf
- `FY_ESP32_Bringup` in PlatformIO öffnen, Build/Upload, Monitor auf 115200 Baud.
- In der Terminalkonfiguration CR oder LF als Zeilenende einschalten.
- `GET_ERROR`, `GET_POSITION`, `HELP` prüfen; bei gültiger Übernahme blinkt GPIO2 einmal je Kommando.
- `SET_TRACK 3` ist nur ein Parser/Queue-Test, **keine echte Bewegung**.
- `BABELFISH OFF` ist **noch nicht implementiert**; optionales Enhancement erst nach Sicherung der bestehenden Betriebsweise.
- Für eine echte binäre Host-Verbindung muss später die klare Moduswahl ASCII/Binär definiert werden, ohne den alten Frame zu ändern.

Keine STEP-, DIR- oder ENABLE-Ausgänge werden in diesem Test benutzt.
