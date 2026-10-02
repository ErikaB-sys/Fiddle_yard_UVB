# FY Produktivsoftware – technische Roadmap

> Arbeitsdiagramm für die schrittweise Überführung der Erkenntnisse aus dem FY_HW_TEST in die Produktivsoftware.

```mermaid
flowchart TD
    A["HW-Test Erkenntnisse<br/>#105"] --> B["Motor / Profil<br/>#104"]
    B --> C["Motor Execution / STEP<br/>#51"]
    B -. parallel .-> T["Tests / Native<br/>#46 + PC-Tests"]
    C --> D["Motorzustand / Sicherheit<br/>#86 + #26 + #50 + #52"]
    D --> E["Sensoren<br/>#93"]
    E --> F["Referenz<br/>#85 + #94"]
    F --> G["Parameter & Kalibrierung<br/>#106"]
    G --> H["Position / Trackmodell<br/>#11"]
    H --> I["Keyboard / LOCAL<br/>#88 + #89"]
    I --> J["Display<br/>#90"]
    I --> K["UART / Service<br/>#13 + #17 + #28 + #29 + #39"]
    J --> L["LOCAL / REMOTE<br/>#91"]
    K --> L
    K -. Service .-> S["Servicebetrieb<br/>#95"]
    L --> M["System Executive<br/>#96"]
    M --> N["Gesamtintegration<br/>#30 + Tests"]
    T -. Validierung .-> D
    T -. Validierung .-> F
    T -. Validierung .-> G
    T -. Validierung .-> N
```

## Arbeitsprinzip

1. Motor / Profil zuerst produktiv belastbar machen.
2. Darauf Motorzustand, Sensoren und Referenz aufbauen.
3. Danach Parameter-, Kalibrier- und Positionsmodell festlegen.
4. Keyboard, Display und UART auf dem stabilen Kern aufsetzen.
5. Servicefunktionen bewusst vom normalen Bedienablauf trennen.
6. Den System Executive erst einführen, wenn die darunterliegenden Module ausreichend stabil sind.
7. Tests parallel laufen lassen und die einzelnen Schritte begleiten.

## Regeln

- Der HW-Test-Code wird nicht kopiert.
- Übernommen werden bestätigte Anforderungen, Zustände, Abläufe, Parameter und technische Prinzipien.
- UART bleibt bei jedem Entwicklungsschritt funktionsfähig, damit der Nano weiterhin beobachtet und getestet werden kann.
- Referenzkompensation zunächst nicht erzwingen; die Architektur dafür bleibt offen.
- Parameter und EEPROM werden in #106 schrittweise klassifiziert.
- `response()` der Produktiv-UART wird im Zuge der UART-Überarbeitung konkretisiert.
- Neue Issues nur anlegen, wenn aus der Umsetzung tatsächlich eine Lücke entsteht.