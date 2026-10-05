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

Vor jedem MISRA-Lauf erzeugt das Script automatisch eine frische
`compile_commands.json` über PlatformIO. Dadurch verwendet Cppcheck die
tatsächlichen Compiler-Defines, Include-Pfade und Toolchain-Einstellungen des
aktiven Build-Umfelds statt einer nachgebildeten Konfiguration.

Cppcheck analysiert anschließend diese PlatformIO-Kompilationsdaten über:

```text
cppcheck --project=.cppcheck/compile_commands.json
```

Die automatische Erzeugung ist bewusst Teil des Build-Gates. Eine manuelle
Pflege der Cppcheck-Defines ist damit nicht erforderlich.

## Ausgabe

- `reports/misra.csv` – versionierter Nachweis der relevanten Findings
- `.cppcheck/` – temporärer Cppcheck-Cache und automatisch erzeugte
  Compilation Database

## Bewertung der Findings

- `misra-c2012-*` → echter MISRA-Verstoß → Build wird abgebrochen
- andere `error`-Findings → echter Cppcheck-Fehler → Build wird abgebrochen
- `misra-config` → Konfigurationsdiagnose → kein Build-Abbruch
- normale Warnungen, Style- und Performance-Hinweise → sichtbar in der
  vollständigen Cppcheck-Ausgabe, aber kein Build-Abbruch

Damit sollen echte Codeprobleme sichtbar werden, ohne Konfigurationsprobleme
mit Codefehlern zu vermischen.

## Initiale Einordnung

Der erste Lauf dient der Bestandsaufnahme. Bestehende Meldungen werden zunächst
klassifiziert und nicht pauschal unterdrückt.

Ziel ist ein reproduzierbarer lokaler Prüfcheck für den produktiven Code unter
`FY_Controller_Motor`.

## Abgrenzung

- Kein GitHub Actions / CI-Check.
- Bestehende VS-Code-Prüfungen bleiben erhalten.
- `FY_HW_TEST` wird nicht verändert.
