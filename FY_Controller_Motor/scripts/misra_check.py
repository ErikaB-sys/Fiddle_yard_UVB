Import("env")

import os
import shutil
import subprocess
import sys

print("\n🔍 Running MISRA / Cppcheck before build...\n")

project_dir = env.subst("$PROJECT_DIR")
reports_dir = os.path.join(project_dir, "reports")
cppcheck_dir = os.path.join(project_dir, ".cppcheck")

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
    "--error-exitcode=1",
    "--template={file},{line},{severity},{id},{message}",
    "--output-file=reports/misra.csv",
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
    print("\n❌ MISRA / Cppcheck failed. Build aborted.\n")
    sys.exit(1)

print("\n✅ MISRA / Cppcheck passed. See reports/misra.csv\n")
