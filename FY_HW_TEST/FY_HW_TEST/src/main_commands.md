# FY HW TEST – serielle Befehle

115200 Baud, Befehle mit CR/LF abschließen.

## Geschwindigkeit

| Befehl | Bedeutung |
|---|---|
| Vnnnn | VMAX in steps/s |
| vnnnn | VMIN in steps/s |

## Rampe

| Befehl | Bedeutung |
|---|---|
| A1n | Beschleunigungswert A1 |
| A2n | Beschleunigungswert A2 |
| B1n | Bremswert B1 |
| B2n | Bremswert B2 |
| A1Nn | Rampenintervall A1 |
| A2Nn | Rampenintervall A2 |
| B1Nn | Rampenintervall B1 |
| B2Nn | Rampenintervall B2 |

Bei den Rampenintervallen bedeutet N=1 Änderung nach jedem STEP, N=2 nach jedem zweiten STEP usw.

## Bewegung

| Befehl | Bedeutung |
|---|---|
| Snnnn | Bewegungsstrecke in Steps |

GO / LEFT / RIGHT / STOP erfolgen beim aktuellen HW-Test über die Taster. Die serielle Schnittstelle dient für Parameter und Referenztest.

## Referenzlauf

| Befehl | Bedeutung |
|---|---|
| R oder r | Referenzlauf starten |
| K oder k | Referenzlauf starten |

Vor R / K muss der Motor mit dem GO-Taster aktiviert sein.

## Referenz-Messdaten

Der Referenzlauf ermittelt schrittweise:

- grobe Position der Referenzfahne beim Lauf L → R
- Position der rechten Fahnenkante beim langsamen Rücklauf R → L
- Position der linken Fahnenkante beim langsamen Rücklauf
- Fahnenlänge in Steps
- Mittelpunkt der Fahne
- Abweichung der groben Position vom gemessenen Mittelpunkt
- Position der rechten Endlage
- nutzbare Strecke zwischen linker und rechter Endlage

Die Ergebnisse werden nach Abschluss des Messlaufs einmal kompakt über UART ausgegeben.

## Speicherprinzip

Serielle Texte im AVR-Code werden mit F("...") im Flash gehalten. Die Messwerte selbst werden nur als numerische Werte gespeichert; keine dynamischen Strings.
