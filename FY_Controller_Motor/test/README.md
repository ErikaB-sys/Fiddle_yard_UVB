# Tests
This directory is intended for PlatformIO Test Runner and project tests.

Unit Testing is a software testing method by which individual units of
source code, sets of one or more MCU program modules together with associated
control data, usage procedures, and operating procedures, are tested to
determine whether they are fit for use. Unit testing finds problems early
in the development cycle.

More information about PlatformIO Unit Testing:
- https://docs.platformio.org/en/latest/advanced/unit-testing/index.html
## Teststruktur

Die Tests werden nach Modulen organisiert.

```text
 test/
 ├── Motor/
 │   ├── test_calcProfile.cpp
 │   ├── test_prepareParameter.cpp
 │   └── test_State.cpp
 └── UART/
     ├── test_Command.cpp
     └── test_CRC.cpp
```

### Regeln

- Das Verzeichnis unter `test/` bezeichnet das Modul, zu dem der Test gehört.
- Der Dateiname beschreibt eindeutig den Testgegenstand, z. B. `test_calcProfile.cpp` für `calcProfile()`.
- Ein Testfile behandelt einen klar abgegrenzten Testgegenstand.
- Mehrere einzelne Testfälle dürfen innerhalb eines Testfiles vorhanden sein.
- Die Produktionsimplementierung wird nicht verändert, nur um einen Test zu ermöglichen. Tests passen sich an die bestehende bzw. bewusst definierte Produktionsschnittstelle an.
- Private interne Daten bleiben privat. Ein Test darf keine Produktionsschnittstelle allein deshalb aufbrechen, damit er interne Implementierungsdetails direkt auslesen kann.
