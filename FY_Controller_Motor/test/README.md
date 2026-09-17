# Tests

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
