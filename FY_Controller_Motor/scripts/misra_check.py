Import("env")

import csv
import os
import shutil
import subprocess
import sys

print("\n🔍 Running MISRA / Cppcheck before build...\n")

project_dir = env.subst("$PROJECT_DIR")
reports_dir = os.path.join(project_dir, "reports")
cppcheck_dir = os.path.join(project_dir, ".cppcheck")
full_report_file = os.path.join(cppcheck_dir, "misra_full.csv")
report_file = os.path.join(reports_dir, "misra.csv")

os.makedirs(reports_dir, exist_ok=True)
os.makedirs(cppcheck_dir, exist_ok=True)

cppcheck = shutil.which("cppcheck")

if cppcheck is None and os.name == "nt":
    windows_cppcheck = r"C:\Program Files\Cppcheck\cppcheck.exe"
    if os.path.isfile(windows_cppcheck):
        cppcheck = windows_cppcheck

if cppcheck is None:
    print("❌ Cppcheck not found. MISRA check aborted.")
    sys.exit(1)

cmd = [
    cppcheck,
    "--enable=warning,style,performance",
    "--addon=misra",
    "--language=c++",
    "--std=c++11",
    "--inline-suppr",
    "--template={file},{line},{severity},{id},{message}",
    "--output-file=.cppcheck/misra_full.csv",
    "--cppcheck-build-dir=.cppcheck",
    "-Iinclude",
    "-Isrc",
    "-Ilib",
    "-DARDUINO",
    "-DARDUINO_AVR_NANO",
    "src",
    "include",
    "lib",
]

result = subprocess.call(cmd, cwd=project_dir)

if result != 0:
    print("\n❌ Cppcheck itself failed. Build aborted.\n")
    sys.exit(1)

misra_findings = 0
error_findings = 0

with open(full_report_file, newline="", encoding="utf-8") as source:
    with open(report_file, "w", newline="", encoding="utf-8") as target:
        writer = csv.writer(target, lineterminator="\n")

        for row in csv.reader(source):
            if len(row) < 5:
                continue

            severity = row[2].strip().lower()
            finding_id = row[3].strip().lower()
            is_misra = finding_id.startswith("misra-c2012-")
            is_config = finding_id == "misra-config"

            if severity == "error" or is_misra:
                writer.writerow(row)

            if is_misra:
                misra_findings += 1
            elif severity == "error" and not is_config:
                error_findings += 1

if misra_findings > 0:
    print(
        f"\n❌ {misra_findings} MISRA issue(s) found. "
        "Build aborted. See reports/misra.csv.\n"
    )
    sys.exit(1)

if error_findings > 0:
    print(
        f"\n❌ {error_findings} error(s) found. "
        "Build aborted. See reports/misra.csv.\n"
    )
    sys.exit(1)

print("\n✅ MISRA / Cppcheck passed. See reports/misra.csv\n")
