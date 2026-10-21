#!/usr/bin/env python3
"""FY ESP32: MISRA-orientierter Cppcheck-Lauf auf eigenen Quellen.

Aus dem Nano-Ansatz abgeleitet. Für C++ KEIN vollständiger MISRA-C++-Nachweis.
Lokal: python scripts/misra_check.py  (cppcheck muss im PATH liegen).
"""
import csv
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
REPORTS = ROOT / "reports"
CPPDIR = ROOT / ".cppcheck"
SOURCES = ["src"] + [
    str(p.relative_to(ROOT)).replace("\\", "/")
    for p in sorted((ROOT / "lib").iterdir())
    if p.is_dir() and any(p.rglob("*.cpp"))
]
INCLUDES = ["include", "src"] + [
    str(p.relative_to(ROOT)).replace("\\", "/")
    for p in sorted((ROOT / "lib").iterdir())
    if p.is_dir()
]

def main():
    exe = shutil.which("cppcheck")
    if not exe and os.name == "nt":
        alt = Path(r"C:\Program Files\Cppcheck\cppcheck.exe")
        if alt.is_file():
            exe = str(alt)
    if not exe:
        print("Cppcheck nicht gefunden (PATH bzw. Standard-Windows-Pfad).")
        return 2
    REPORTS.mkdir(exist_ok=True)
    CPPDIR.mkdir(exist_ok=True)
    cmd = [
        exe, "--enable=warning,style,performance", "--addon=misra",
        "--language=c++", "--std=c++17", "--inline-suppr",
        "--suppress=missingIncludeSystem",
        "--template={file},{line},{severity},{id},{message}",
        "--output-file=reports/misra_full.csv", "--cppcheck-build-dir=.cppcheck",
        "-DARDUINO", "-DARDUINO_ARCH_ESP32", "-DFY_ESP32_BRINGUP=1",
        "-DFY_MOTOR_BENCH_TEST=1",
    ]
    cmd.extend("-I" + name for name in INCLUDES)
    cmd.extend(SOURCES)
    print("ESP32: MISRA-orientierte statische Analyse, nur FY-eigene Quellen")
    rc = subprocess.call(cmd, cwd=ROOT)
    if rc:
        print("Cppcheck selbst schlug fehl:", rc)
        return rc
    full = REPORTS / "misra_full.csv"
    keep = REPORTS / "misra.csv"
    findings = []
    config = 0
    with full.open(newline="", encoding="utf-8", errors="replace") as f:
        for row in csv.reader(f):
            if len(row) < 5:
                continue
            p = Path(row[0])
            abs_path = (p if p.is_absolute() else ROOT / p).resolve()
            try:
                abs_path.relative_to(ROOT)
            except ValueError:
                continue  # Framework-/Fremdcode ausschließen
            sev, rule = row[2].strip().lower(), row[3].strip().lower()
            if rule == "misra-config":
                config += 1
            if rule.startswith("misra-c2012-") or sev == "error":
                findings.append(row)
    with keep.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f, lineterminator="\n")
        writer.writerows(findings)
    print(f"Ergebnis: {len(findings)} relevante Befunde, {config} misra-config-Hinweise.")
    print("Berichte:", full, "und", keep)
    print("WICHTIG: MISRA C:2012 auf C++ ist KEIN vollständiger MISRA-C++-Check.")
    return 1 if findings else 0

if __name__ == "__main__":
    sys.exit(main())
