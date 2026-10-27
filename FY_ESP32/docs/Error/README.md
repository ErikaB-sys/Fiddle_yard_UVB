# Error – zentrale Diagnosesicht (Huginn)

**Soll:** [#158](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/158), [#162](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/162), [#155](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/155).

Fehlercode: hohes Nibble = Ort, niedriges Nibble = Grund. Kritische Muninn-Fehler führen **zuerst lokal** zum Schutz; erst danach zu Meldung/Diagnose auf Huginn. Keine konkurrierenden Schreibzugriffe auf globale Fehlereinträge.

```mermaid
flowchart LR
  M["Muninn: Fehler + Schutz"] -.-> Q["geplanter Fehler-Snapshot"]
  Q -.-> E["Huginn: Diagnose-Owner"]
  U["UART-Fehler"] --> E
  E --> R["STATUS_Error / später OLED"]
```

**Ist:** [Error.h/.cpp](../../lib/Error/Error.h) sind bereits portiert, und `UART` besitzt eine `Error _errors`-Instanz. Anders als im alten Dummy-Dokument ist Error also **nicht völlig unportiert**. Eine einheitliche Core-übergreifende Aggregation ist jedoch nicht nachgewiesen.

- [x] Tatsächlichen Code benannt.
- [ ] Fehlerzustand und Synchronisation Muninn → Huginn definieren.
- [ ] Fehlerformat und Reset-/Clear-Verhalten Ende-zu-Ende prüfen.
- [ ] HW-Fehlerfälle/MISRA-/Reviewstand nachweisen.

Offen: [VERIFICATION_GAPS.md](../VERIFICATION_GAPS.md). Altstand [archive/Error.md](archive/Error.md).

## Daten- und Funktionsdiagramme (Release 2.0.0)

## Fehler- und Rückmeldungsfluss

```mermaid
flowchart LR
  B["BabelFish / UART"] --> E["Fehlerstatus"]
  M["Motor / Sensor / NVM"] --> E
  E --> N["NACK Ort und Grund"]
  E --> S["GET_ERROR / STATUS"]
  N --> H["UART-Host"]
  S --> H
```

```mermaid
sequenceDiagram
  participant Host
  participant UART
  participant Fehlerstatus
  Host->>UART: ungültiger Auftrag
  UART->>Fehlerstatus: Fehler registrieren
  UART-->>Host: NACK
  Host->>UART: GET_ERROR
  UART->>Fehlerstatus: Snapshot lesen
  UART-->>Host: STATUS_Error
```

**Zugehörige Issues:** [#150](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/150), [#171](https://github.com/ErikaB-sys/Fiddle_yard_UVB/issues/171).
