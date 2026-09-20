# MOTOR – PREFRUN first version

Erstes Diagramm für den Reference-Run.

Der Flow ist bewusst eine erste Version. Offene Detailentscheidungen bleiben sichtbar und werden vor der Implementierung geklärt.

```mermaid
flowchart TD

    A[Reference aufgerufen?]

    A --> B{Motor IDLE?}

    B -->|Nein| X[Reference ablehnen]

    B -->|Ja| C{Referenz bereits gültig?}

    C -->|Ja| Y[Referenzdaten löschen]

    C -->|Nein| D{END_LEFT aktiv?}

    Y --> D

    D -->|Ja| E[Escape: ca. 30 Steps nach RIGHT]

    E --> I{ENDschalter L frei?}

    I -->|Ja| F[Langsam nach LEFT fahren]

    I -->|Nein| O[REF_ERROR]

    D -->|Nein| F

    F --> G{END_LEFT erreicht?}

    G -->|Ja| H[Position = linker mechanischer Endpunkt]

    G -->|Nein| F

    H --> K[Langsam nach RIGHT fahren]

    K --> L{REFERENCE erkannt?}

    L -->|Ja| M[REFERENCE Position merken]

    L -->|Nein| N{END_RIGHT erreicht?}

    N -->|Nein| K

    N -->|Ja| O

    M --> P[Weiter nach RIGHT bis END_RIGHT]

    P --> Q[Gesamtstrecke ermitteln]

    Q --> R[Bewegungsbereich berechnen]

    R --> S[POS_MIN / POS_MAX festlegen]

    S --> T[Reference gültig]

    T --> U[REF_VALID]
```

## Offen

- Abbruch rechts, wenn der Endschalter nicht innerhalb der mathematisch erwarteten Maximalstrecke gefunden wird.
- Schleife für das Erkennen der Referenzfahne: `LOW / HIGH / LOW`.
- Speichern der Breite der Referenzfahne in Steps.
