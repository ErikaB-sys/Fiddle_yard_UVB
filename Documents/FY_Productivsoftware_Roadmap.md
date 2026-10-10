# FY Produktivsoftware – technische Roadmap

> Arbeitsdiagramm für die schrittweise Überführung der Erkenntnisse aus dem FY_HW_TEST in die Produktivsoftware.

```mermaid
flowchart TD
    A["HW-Test Erkenntnisse<br/>#105"] --> B["Motor / Profil<br/>#104"]
    V["Firmware-Identität<br/>#107/ DONE"] -. Build-/Diagnosebasis .-> B

    B --> C["UART / Intermediate Commands<br/>#13"]
    C --> D["Erstes Positions-Command / Motor-Schnittstelle<br/>#28"]
    D --> E["Parameter / Profilwerte<br/>#106 + #104"]
    E --> F["Motorbewegung / Execution / STEP<br/>#51"]

    F -. parallel .-> T["Tests / Native<br/>#46 + PC-Tests"]
    F --> G["Motorzustand / Sicherheit<br/>#86 + #26 + #50 + #52"]
    G --> H["Sensoren<br/>#93"]
    H --> I["Referenz<br/>#85 + #94"]
    I --> J["Position / Trackmodell<br/>#11"]
    J --> K["Keyboard / LOCAL<br/>#88 + #89"]
    K --> L["Display<br/>#90"]
    K --> M["UART / Service<br/>#13 + #17 + #28 + #29 + #39"]
    L --> N["LOCAL / REMOTE<br/>#91"]
    M --> N
    M -. Service .-> S["Servicebetrieb<br/>#95"]
    N --> O["System Executive<br/>#96"]
    O --> P["Gesamtintegration<br/>#30 + Tests"]

    T -. Validierung .-> G
    T -. Validierung .-> I
    T -. Validierung .-> J
    T -. Validierung .-> P
```

## Arbeitsreihenfolge

Die Motorintegration wird bewusst in kleinen, testbaren Schritten aufgebaut. Die echte Hardware ist dabei zunächst **nicht erforderlich**; die Produktivsoftware wird auf dem Schreibtisch-Mockup entwickelt und geprüft.

1. **Firmware-Identität** als kleine Build-/Diagnosebasis festlegen.
2. **Motor-/Profil-Erkenntnisse aus dem HW-Test** übernehmen, ohne den HW-Test-Code zu kopieren.
3. **Intermediate Commands** zuerst produktiv stabilisieren. Damit wird die Kommunikation CMD → Verarbeitung → Response unabhängig von echter Motorbewegung überprüfbar.
4. **Erstes Positions-Command / Motor-Schnittstelle** einführen. Zunächst reicht die Verbindung bis zur Motorlogik bzw. zum Mockup; die vollständige STEP-Ausführung folgt später.
5. **Parameter- und Profilmodell** schrittweise festlegen. Als Startpunkt werden die im HW-Test verifizierten und veränderbaren Parameterwerte verwendet. Die endgültige Klassifizierung von Default-, Konfigurations-, Kalibrier-, Laufzeit- und Diagnosewerten erfolgt in #106.
6. **Motorbewegung / Execution / STEP** in die Produktivsoftware überführen. Die verifizierte Profilberechnung aus #104 und die STEP-Ausführung aus #51 werden dabei getrennt gehalten: Profil berechnet, Execution führt aus.
7. **Mockup vollständig gegen die Produktivschnittstellen testen.**
8. Erst wenn Kommunikation, Position, Parameter und Motor-Execution auf dem Mockup funktionieren, erfolgt der **erste Live-Test mit Steppertreiber und echter Motor-Hardware**.
9. Danach Motorzustand, Sicherheit, Sensoren und Referenz schrittweise ergänzen.
10. Erst auf diesem stabilen Kern folgen Bedienung, Service und Systemintegration.

## Arbeitsprinzip

- Die drei Command-Klassen werden bewusst nacheinander genutzt: **INTERMEDIATE → Positions-/Bewegungsauftrag → weitere ausführende Commands**.
- Kommunikation wird vor echter Motorbewegung stabilisiert.
- Das erste Positions-Command bildet die Brücke zwischen UART und Motor-Schnittstelle.
- Parameter werden zunächst aus dem HW-Test übernommen, aber ihre endgültige Struktur wird nicht vorweggenommen.
- Die reale Hardware dient erst dann als Integrationsprüfung, wenn die Produktivlogik auf dem Mockup nachvollziehbar funktioniert.
- Der Steppertreiber ist eine nachgelagerte Hardware-Schnittstelle; seine konkrete Implementierung wird nicht zum Vorab-Gegenstand der Motor-API gemacht.
- Der HW-Test-Code wird nicht kopiert.
- Übernommen werden bestätigte Anforderungen, Zustände, Abläufe, Parameter und technische Prinzipien.
- UART bleibt bei jedem Entwicklungsschritt funktionsfähig, damit der Nano weiterhin beobachtet und getestet werden kann.
- Referenzkompensation zunächst nicht erzwingen; die Architektur dafür bleibt offen.
- Parameter und EEPROM werden in #106 schrittweise klassifiziert.
- `response()` der Produktiv-UART wird im Zuge der UART-Überarbeitung konkretisiert.
- Neue Issues nur anlegen, wenn aus der Umsetzung tatsächlich eine Lücke entsteht.
- Die Firmware-Identität wird zentral gepflegt: Release-Version manuell, Builddatum/-zeit und Git-Commit automatisch aus dem Build ableiten.

## Gemeinsame Software-Komponenten und Versionierung

Gemeinsame Dateien oder Bibliotheksbestandteile zwischen HW-Test und Produktivsoftware sind grundsätzlich zulässig, wenn sie tatsächlich ein gemeinsames Verfahren oder einen gemeinsamen Mechanismus beschreiben. Das gilt insbesondere für hardwarefreie Berechnungen oder eindeutig definierte Datenstrukturen.

Dabei gilt:

- Eine gemeinsame Datei ist eine **gemeinsame Software-Komponente**, nicht automatisch Bestandteil jeder Firmware-Version.
- Änderungen an einer gemeinsamen Komponente dürfen **keine stille Verhaltensänderung** in einem anderen Projekt erzeugen.
- Die Übernahme einer geänderten gemeinsamen Komponente in eine Firmware muss deshalb **bewusst und nachvollziehbar** erfolgen.
- Die verwendete Version bzw. der konkrete Git-Stand der Komponente muss aus dem jeweiligen Projektstand nachvollziehbar sein.
- Eine Änderung der gemeinsamen Komponente wird zunächst isoliert geprüft. Erst danach wird sie in die jeweils betroffenen Firmwarebereiche übernommen.
- Wenn HW-Test und Produktivsoftware fachlich auseinanderlaufen, darf eine gemeinsame Datei aufgelöst werden. Beide Projekte erhalten dann bewusst eigene Implementierungen.
- **Submodule oder Subrepositories werden dafür nicht vorausgesetzt.** Die gemeinsame Komponente kann im selben Repository liegen; entscheidend ist die nachvollziehbare Übernahme und nicht die technische Verpackung.
- Vor einer Änderung an `FY_Common` wird daher geprüft, welche Projekte die Komponente tatsächlich verwenden und ob eine gemeinsame Weiterentwicklung noch sinnvoll ist.

Für `FY_Common/MotorProfile.h` ist damit zunächst **keine automatische gemeinsame Weiterentwicklung festgelegt**. Vor der nächsten Änderung wird geklärt, ob die Profilberechnung weiterhin eine echte gemeinsame Komponente sein soll oder ob HW-Test und Produktivsoftware an diesem Punkt bewusst getrennte Wege gehen.

## Branch- und Merge-Regel

Die laufende Produktiventwicklung erfolgt ausschließlich auf dem Branch `FY_Controller_Motor`. `main` ist die gemeinsame stabile Basis und wird während der laufenden Entwicklung nicht parallel bearbeitet.

Die nächste Merge-Schwelle ist erreicht, sobald folgende Punkte abgeschlossen und geprüft sind:

1. Die zentrale Fehlerverwaltung funktioniert einschließlich Setzen, Ersetzen, Löschen und Begrenzung auf drei gespeicherte Fehler.
2. UART `GET_ERROR` liefert Statusbyte und Fehlerplätze korrekt; Telegrammlänge und CRC sind geprüft.
3. Reproduzierbare native PC-Tests für Fehlerverwaltung und Kommunikation bestehen.
4. Der Firmware-Build ist erfolgreich; es bestehen keine bekannten Blocker für diesen abgeschlossenen Zwischenstand.

Dann wird `FY_Controller_Motor` nach `main` gemergt. Die automatische Erkennung unterschiedlicher Platinenvarianten ist dafür keine Voraussetzung; sie bleibt bis zur späteren Umsetzung ein dokumentierter Vorschlag in `HW_Config.h`.

Nach dem Merge werden beide Branches auf eine gemeinsame Basis gebracht. Vor jedem neuen Arbeitsabschnitt werden der aktive Branch und der aktuelle Commit geprüft und als Ausgangspunkt benannt. Es wird nicht stillschweigend zwischen unterschiedlichen Branch-Ständen gewechselt.

