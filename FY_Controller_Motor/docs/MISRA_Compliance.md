# MISRA / Cppcheck

## Zweck

Der FY-Controller verwendet Cppcheck mit dem MISRA-Addon als lokalen Build-Gate.

Der Check läuft als PlatformIO-Pre-Build-Script und muss erfolgreich sein, bevor
der Firmware-Build fortgesetzt wird.

## Ausführung

PlatformIO startet den Check über:

```ini
extra_scripts = pre:scripts/misra_check.py
```

Das Script erwartet eine lokal installierte Cppcheck-Version mit MISRA-Addon.

## Ausgabe

- `reports/misra.csv` – Ergebnisdatei
- `.cppcheck/` – Cppcheck-Cache

Beide Verzeichnisse sind vom Git-Tracking ausgeschlossen.

## Initiale Einordnung

Der erste Lauf dient der Bestandsaufnahme. Bestehende Meldungen werden zunächst
klassifiziert und nicht pauschal unterdrückt.

Ziel ist ein reproduzierbarer lokaler Prüfcheck für den produktiven Code unter
`FY_Controller_Motor`.

## Abgrenzung

- Kein GitHub Actions / CI-Check.
- Bestehende VS-Code-Prüfungen bleiben erhalten.
- `FY_HW_TEST` wird nicht verändert.
