# Zusatztask "OTA Upload" fuer die bestehende esp32dev-Umgebung.
# Keine zweite Firmware-Build-Umgebung: dieselbe firmware.bin wie fuer USB.
Import("env")

import subprocess
import sys
from pathlib import Path


def run_fy_ota(source, target, build_env):
    host = build_env.GetProjectOption("custom_ota_host", "").strip()
    project = Path(build_env.subst("$PROJECT_DIR"))
    firmware = Path(build_env.subst("$BUILD_DIR")) / (
        build_env.subst("${PROGNAME}") + ".bin"
    )
    command = [
        sys.executable,
        "-u",
        str(project / "scripts" / "fy_ota_upload.py"),
        "--host", host,
        "--file", str(firmware),
    ]
    result = subprocess.call(command, cwd=str(project))
    if result != 0:
        raise SystemExit(result)


if env.get("PIOENV") == "esp32dev":
    env.AddCustomTarget(
        name="ota",
        dependencies="$BUILD_DIR/${PROGNAME}.bin",
        actions=run_fy_ota,
        title="OTA Upload",
        description=(
            "Kompiliert bei Bedarf und uebertraegt firmware.bin per WLAN "
            "nach 3 Sekunden physischer STOP-Freigabe"
        ),
    )
